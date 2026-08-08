#include <stddef.h>
#include <stdint.h>
#define PROTOCOL_IMPLEMENTATION
#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEF_IP "127.0.0.1"
#define DEF_PORT 8080

bool is_num(char *c_str) {
  char *x = c_str;
  while (*x) {
    if (*x < '0' || *x > '9')
      return false;
    x++;
  }
  return true;
}

int main(int argc, char *argv[]) {
  char *ip_addr = DEF_IP;
  int port = DEF_PORT;
  if (argc == 2) {
    ip_addr = argv[1];
  } else if (argc == 3) {
    ip_addr = argv[1];
    if (!is_num(argv[2])) {
      printf("Usage : %s <ip-addr> <port>\nProvided port %s isn't valid\n",
             argv[0], argv[2]);
      return 1;
    }
    port = atoi(argv[2]);
  }

  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_port = htons(port),
                             .sin_addr.s_addr = inet_addr(ip_addr)};
  socklen_t addr_len = sizeof(addr);

  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    printf("error creating socket\n");
    return 1;
  }

  if (connect(server, (struct sockaddr *)&addr, addr_len) < 0) {
    printf("error connecting to the server : %s\n", strerror(errno));
    return 1;
  }

  // handle connection
  printf("Connected to server\n");
  request req = create_req_from_cstr(REQ_LOGIN, "HELLO WORLD!");
  send_req(server, req);
  printf("request sent!");

  close(server);

  return 0;
}
