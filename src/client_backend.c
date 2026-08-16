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
                     .status = CLIENT_NOT_LOGGED_IN,
                     .user_search_results = NULL,
                     .current_dm = NULL,
                     .dm_messages = NULL
  };
  return out;
}

void close_client(client_data cd) {
  SSL_shutdown(cd.server_ssl);
  SSL_free(cd.server_ssl);
  close(cd.server_fd);
}

bool compare_req(request req, const char *str){
  if (req.length != strlen(str)) return false;
  for(size_t i = 0; i < req.length; i++){
    if(req.data[i] != str[i]) return false;
  }
  return true;
}

void create_account(client_data *cd,  char *username, const char *password){
  char *data = malloc(strlen(username) + strlen(password) + 1);
  for(size_t i = 0; i < strlen(username); i++) data[i] = username[i];
  data[strlen(username)] = 0;
  for(size_t i = 0; i <strlen(password); i++) data[i + strlen(username) + 1] = password[i];
  request req = create_req_from_cstr(REQ_SIGN_UP, data);
  send_req(cd->server_ssl, req);
  destroy_request(&req);
  request rsp = recv_req(cd->server_ssl);
  if (rsp.kind == RSP_SIGN_UP && compare_req(rsp, "OK")) {
    cd->username = username;
    cd->logged_in = true;
    cd->status = CLIENT_USERNAME_TAKEN;
  } else if (rsp.kind == RSP_SIGN_UP && compare_req(req, "IN USE")){
    cd->status = CLIENT_INVALID_INPUT; // Username already in use
  } else if (rsp.kind == RSP_ERROR){
    cd->status = CLIENT_SERVER_ERROR; // Server side error
  }
}

void login(client_data *cd, char *username, const char *password){
  char *data = malloc(strlen(username) + strlen(password) + 1);
  for(size_t i = 0; i < strlen(username); i++) data[i] = username[i];
  data[strlen(username)] = 0;
  for(size_t i = 0; i <strlen(password); i++) data[i + strlen(username) + 1] = password[i];
  request req = create_req_from_cstr(REQ_LOGIN, data);
  send_req(cd->server_ssl, req);
  destroy_request(&req);
  request rsp = recv_req(cd->server_ssl);
  if (rsp.kind == RSP_LOGIN && compare_req(rsp, "OK")) {
    cd->username = username;
    cd->logged_in = true;
    cd->status = CLIENT_USERNAME_TAKEN;
  } else if (rsp.kind == RSP_LOGIN && compare_req(rsp, "WRONG")){
    // incorrect username or password
    cd->status =   CLIENT_AUTH_FAILED;
  } else if (rsp.kind == RSP_ERROR){
    cd->status = CLIENT_SERVER_ERROR; // Server side error
  }
}

void logout(client_data *cd){
  request req = create_req_from_cstr(REQ_LOGOUT, "");
  send_req(cd->server_ssl, req);
  destroy_request(&req);
  request rsp = recv_req(cd->server_ssl);
  if (rsp.kind == REQ_LOGOUT && compare_req(rsp, "OK")){
    cd->status = CLIENT_NOT_LOGGED_IN;
    cd->logged_in = false;
    cd->username = NULL;
    if(cd->user_search_results){
      free(cd->user_search_results);
      cd->user_search_results = NULL;
    }
    cd->current_dm = NULL;
    if(cd->dm_messages.msgs) {
      for(size_t i = 0; i < cd->dm_messages.size; i++) destroy_message(&cd->dm_messages.msgs[i]);
      free(cd->dm_messages.msgs);
    }
    cd->dm_messages.msgs = NULL;
  } else if (rsp.kind == RSP_ERROR){
    cd->status = CLIENT_SERVER_ERROR; // Server side error
  }
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
