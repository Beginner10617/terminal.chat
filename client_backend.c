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

int main(int argc, char *argv[]) {
  char *ip_addr = DEF_IP;
  int port = DEF_PORT;
  if (argc == 2) {
    ip_addr = argv[1];
  } else if (argc == 3) {
    ip_addr = argv[1];
    if (!is_num(argv[2])) {
      LOG_ERROR("Usage : %s [<ip-addr>] [<port>]\nProvided port %s isn't valid",
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
    LOG_ERROR("error creating socket");
    return 1;
  }

  if (connect(server, (struct sockaddr *)&addr, addr_len) < 0) {
    LOG_ERROR("error connecting to the server : %s", strerror(errno));
    return 1;
  }

  // handle connection
  LOG_INFO("connected to server");
  request req = create_req_from_cstr(REQ_LOGIN, "HELLO WORLD!");
  send_req(server, req);
  LOG_INFO("request sent!");

  close(server);

  return 0;
}
