#include <poll.h>
#include <stdint.h>
#define PROTOCOL_IMPLEMENTATION
#include "protocol.h"
#include <arpa/inet.h>
#include <assert.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096 // 4 kb
#define TIMEOUT_MS 5

typedef struct {
  int file_descriptor;
  struct sockaddr_storage addr;
  socklen_t addr_len;
  char buf[BUFFER_SIZE];
  bool is_connected;
} client;

typedef struct {
  client *clients;
  size_t size, cap;
} dyn_arr_client;

void init_dyn_arr_client(dyn_arr_client *arr) {
  arr->clients = malloc(sizeof(client));
  arr->size = 0;
  arr->cap = 1;
}

void append_dyn_arr_client(client _client, dyn_arr_client *arr) {
  if (arr->size >= arr->cap) {
    while (arr->size >= arr->cap)
      arr->cap *= 2;
    arr->clients = realloc(arr->clients, sizeof(client) * arr->cap);
  }
  arr->clients[arr->size] = _client;
  arr->size++;
}

void clear_dyn_arr_client(dyn_arr_client *arr) { // remove all disconnected
  ssize_t last_connected = arr->size - 1;
  while (last_connected >= 0 && !arr->clients[last_connected].is_connected)
    last_connected--;
  for (ssize_t curr = last_connected; curr >= 0; curr--) {
    if (!arr->clients[curr].is_connected) {
      close(arr->clients[curr].file_descriptor);
      client tmp = arr->clients[last_connected];
      arr->clients[last_connected] = arr->clients[curr];
      arr->clients[curr] = tmp;
      last_connected--;
    }
  }
  arr->size = last_connected + 1;
}

int main(int argc, char *argv[]) {
  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    printf("error creating socket\n");
    return 1;
  }
  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
      .sin_addr.s_addr = INADDR_ANY // 0.0.0.0, all interfaces
  };

  int tmp_i = 1;
  if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &tmp_i, sizeof(tmp_i)) < 0) {
    printf("error setsockopt : %s\n", strerror(errno));
    return 1;
  }

  if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("error binding : %s\n", strerror(errno));
    return 1;
  }
  if (listen(server, SOMAXCONN) < 0) { // max 5 connection queue
    printf("error listening : %s\n", strerror(errno));
    return 1;
  }

  dyn_arr_client client_list;
  init_dyn_arr_client(&client_list);

  printf("Server running on port %d\n", PORT);
  struct ifaddrs *if_addr;
  if (getifaddrs(&if_addr) < 0) {
    printf("error getifaddrs : %s\n", strerror(errno));
  }
  for (struct ifaddrs *ifa = if_addr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, ip,
                sizeof(ip));
      printf("%s: %s\n", ifa->ifa_name, ip);
    }
  }

  while (true) {
    size_t fd_count = client_list.size + 1;
    struct pollfd all_fd[fd_count];
    all_fd[0].fd = server;
    all_fd[0].events = POLLIN;

    for (size_t i = 1; i < fd_count; i++) {
      all_fd[i].fd = client_list.clients[i - 1].file_descriptor;
      all_fd[i].events = POLLIN;
    }

    if (poll(all_fd, fd_count, TIMEOUT_MS) < 0) {
      printf("error poll : %s\n", strerror(errno));
      continue;
    }

    if (all_fd[0].revents & POLLIN) {
      client tmp_client;
      tmp_client.addr_len = sizeof(tmp_client.addr);
      tmp_client.file_descriptor = accept(
          server, (struct sockaddr *)&tmp_client.addr, &tmp_client.addr_len);
      if (tmp_client.file_descriptor < 0) {
        continue;
      }
      tmp_client.is_connected = true;
      append_dyn_arr_client(tmp_client, &client_list);
      printf("Connected client with file_descriptor = %d\n",
             tmp_client.file_descriptor);
    }
    for (size_t i = 1; i < fd_count; i++) {
      if (!client_list.clients[i - 1].is_connected)
        continue;
      if (all_fd[i].revents & POLLIN) {
        printf("Recieved request\n");
        request req = recv_req(all_fd[i].fd);
        if (req.kind == DISCONNECT) {
          client_list.clients[i - 1].is_connected = false;
          printf("Disconnected fd = %d\n", all_fd[i].fd);
        }
        debug_print_request(req);
      }
    }
    clear_dyn_arr_client(&client_list);
  }

  return 0;
}
