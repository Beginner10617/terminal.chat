#include <openssl/err.h>
#include <openssl/ssl.h>
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

  OPENSSL_init_ssl(0, NULL);
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == NULL) {
    ERR_print_errors_fp(stderr);
    exit(1);
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

  SSL *ssl = SSL_new(ctx);

  if (ssl == NULL) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  SSL_set_fd(ssl, server);

  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  // handle connection
  LOG_INFO("connected to server");
  request req = (request){REQ_PING, 0, NULL};
  send_req(ssl, req);
  LOG_INFO("ping request sent!");
  request pong = recv_req(ssl);
  LOG_INFO("recieved response:");
  debug_request(pong);

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(server);

  return 0;
}
