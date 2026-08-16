#include "client_backend.h"
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

client_data init(const char *ip, unsigned int port) {
  OPENSSL_init_ssl(0, NULL);
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == NULL) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_port = htons(port),
                             .sin_addr.s_addr = inet_addr(ip)};
  socklen_t addr_len = sizeof(addr);

  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    LOG_ERROR("error creating socket");
    exit(1);
  }

  if (connect(server, (struct sockaddr *)&addr, addr_len) < 0) {
    LOG_ERROR("error connecting to the server : %s", strerror(errno));
    exit(1);
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
  client_data out = {.server_ssl = ssl,
                     .server_fd = server,
                     .username = NULL,
                     .logged_in = false,
                     .status = CLIENT_NOT_LOGGED_IN};
  return out;
}

void close_client(client_data cd) {
  SSL_shutdown(cd.server_ssl);
  SSL_free(cd.server_ssl);
  close(cd.server_fd);
}

void create_account(client_data *cd, const char *username, const char *password){
  
}

#define DEF_IP "127.0.0.1"
#define DEF_PORT 8080

int main(int argc, char *argv[]) {
  char *ip_addr = DEF_IP;
  int port = DEF_PORT;
  if (argc == 2) {
    ip_addr = argv[1];
  } else if (argc == 3) {
    ip_addr = argv[1];
    port = atoi(argv[2]);
  }
  client_data client = init(ip_addr, port);
  // handle connection
  LOG_INFO("connected to server");
  request req = (request){REQ_PING, 0, NULL};
  send_req(client.server_ssl, req);
  LOG_INFO("ping request sent!");
  request pong = recv_req(client.server_ssl);
  LOG_INFO("recieved response:");
  debug_request(pong);

  close_client(client);
  return 0;
}
