#include "hashmap.h"
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

typedef struct {
  int file_descriptor;
  struct sockaddr_storage addr;
  socklen_t addr_len;
  char buf[BUFFER_SIZE];
  bool is_connected;
} client;

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
  HashMap *addr_map = hashmap_create(16);

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
    client *tmp_client = malloc(sizeof(client));
    tmp_client->addr_len = sizeof(tmp_client->addr);
    tmp_client->file_descriptor = accept(
        server, (struct sockaddr *)&tmp_client->addr, &tmp_client->addr_len);
    if (tmp_client->file_descriptor < 0) {
      free(tmp_client);
      continue;
    }
    tmp_client->is_connected = true;
    hashmap_put(addr_map, tmp_client->file_descriptor, tmp_client);
    printf("Connected client with file_descriptor = %d\n",
           tmp_client->file_descriptor);

    // process each client in the hashmap
    request req = recv_req(tmp_client->file_descriptor);
    printf("Recieved request\n");
    debug_print_request(req);
  }

  return 0;
}
