#include <poll.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#define PROTOCOL_IMPLEMENTATION
#include "protocol.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define TIMEOUT_MS 5
#define DEF_THREADS 0

typedef struct {
  int file_descriptor;
  struct sockaddr_storage addr;
  socklen_t addr_len;
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
    client *tmp_c = realloc(arr->clients, sizeof(client) * arr->cap);
    if (tmp_c == NULL) {
      LOG_ERROR("unable to allocate space for new clients");
      return;
    }
    arr->clients = tmp_c;
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

typedef struct {
  request_queue que;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  size_t t_id;
} worker_args;

size_t get_worker_thread(int file_descriptor, int num_threads) {
  return file_descriptor % num_threads;
}

void *worker_process(void *arg) {
  worker_args *w_args = arg;
  while (1) {
    pthread_mutex_lock(&w_args->mutex);
    while (is_empty_request_queue(w_args->que))
      pthread_cond_wait(&w_args->cond, &w_args->mutex);
    request req = front_request_queue(w_args->que);
    LOG_INFO("t_id : %zu queue size : %zu ", w_args->t_id, w_args->que.size);
    process_request(req);
    pop_request_queue(&w_args->que);
    pthread_mutex_unlock(&w_args->mutex);
  }
}

int main(int argc, char *argv[]) {
  int n_threads = DEF_THREADS;
  if (argc == 2) {
    if (!is_num(argv[1])) {
      LOG_ERROR("Usage : %s [n_threads]\nProvided n_threads %s isn't valid",
                argv[0], argv[1]);
      return 1;
    }
    n_threads = atoi(argv[1]);
  }

  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    LOG_ERROR("error creating socket");
    return 1;
  }
  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
      .sin_addr.s_addr = INADDR_ANY // 0.0.0.0, all interfaces
  };

  int tmp_i = 1;
  if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &tmp_i, sizeof(tmp_i)) < 0) {
    LOG_ERROR("error setsockopt : %s", strerror(errno));
    return 1;
  }

  if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    LOG_ERROR("error binding : %s", strerror(errno));
    return 1;
  }
  if (listen(server, SOMAXCONN) < 0) { // max 5 connection queue
    LOG_ERROR("error listening : %s", strerror(errno));
    return 1;
  }

  dyn_arr_client client_list;
  init_dyn_arr_client(&client_list);
  request_queue sequential_que;
  if (!n_threads)
    init_request_queue(&sequential_que);

  worker_args args[n_threads];
  for (size_t i = 0; i < n_threads; i++) {
    init_request_queue(&args[i].que);
    pthread_cond_init(&args[i].cond, NULL);
    pthread_mutex_init(&args[i].mutex, NULL);
    args[i].t_id = i + 1;
  }

  pthread_t threads[n_threads];

  for (size_t i = 0; i < n_threads; i++) {
    pthread_create(&threads[i], NULL, worker_process, &args[i]);
  }

  LOG_INFO("Server running on port %d", PORT);
  struct ifaddrs *if_addr;
  if (getifaddrs(&if_addr) < 0) {
    LOG_ERROR("error getifaddrs : %s", strerror(errno));
  }
  for (struct ifaddrs *ifa = if_addr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, ip,
                sizeof(ip));
      LOG_INFO("%s: %s", ifa->ifa_name, ip);
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
      LOG_ERROR("error poll : %s", strerror(errno));
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
      LOG_INFO("connected client with file_descriptor = %d",
               tmp_client.file_descriptor);
    }
    for (size_t i = 1; i < fd_count; i++) {
      client *tmp_client = &client_list.clients[i - 1];
      if (!tmp_client->is_connected)
        continue;
      if (all_fd[i].revents & POLLIN) {
        request req = recv_req(all_fd[i].fd);
        req.file_descriptor = all_fd[i].fd;
        if (req.kind == DISCONNECT)
          tmp_client->is_connected = false;

        if (n_threads) {
          size_t t_id =
              get_worker_thread(tmp_client->file_descriptor, n_threads);
          pthread_mutex_lock(&args[t_id].mutex);
          push_request_queue(&args[t_id].que, req);
          pthread_cond_signal(&args[t_id].cond);
          pthread_mutex_unlock(&args[t_id].mutex);
        } else {
          push_request_queue(&sequential_que, req);
        }
      }
    }

    if (n_threads == 0) {
      while (!is_empty_request_queue(sequential_que)) {
        request req = front_request_queue(sequential_que);
        process_request(req);
        pop_request_queue(&sequential_que);
      }
    }
    clear_dyn_arr_client(&client_list);
  }

  return 0;
}
