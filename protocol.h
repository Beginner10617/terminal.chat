#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>
#define LOG_INFO(format, ...)                                                  \
  fprintf(stderr, "[INFO] " format "\n", ##__VA_ARGS__)
#define LOG_ERROR(format, ...)                                                 \
  fprintf(stderr, "[ERROR] " format "\n", ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                                 \
  fprintf(stderr, "[DEBUG] " format "\n", ##__VA_ARGS__)

typedef enum {
  // disconnect
  DISCONNECT,
  // login for existing users
  REQ_LOGIN,
  RSP_LOGIN,
  // signup for new users
  REQ_SIGN_UP,
  RSP_SIGN_UP,
  // logout
  REQ_LOGOUT,
  RSP_LOGOUT,
  // search other users to chat
  REQ_SEARCH_USER,
  RSP_SEARCH_USER,
  // request user info or a particular username (bio etc)
  REQ_USER_INFO,
  RSP_USER_INFO,
  // send message to other users
  REQ_MESSAGE_SEND,
  RSP_MESSAGE_SEND,
  // list all the people you've chatted with recently
  REQ_CHAT_ACCESS,
  RSP_CHAT_ACCESS,
  // open your dms with a particular user
  REQ_DM_ACCESS,
  RSP_DM_ACCESS,
  // pinging
  REQ_PING,
  RSP_PONG,
  // generic error
  RSP_ERROR,
} REQUEST_RESPONSE_KIND;

typedef struct {
  uint8_t kind, length, *data;
} request;

char *debug_c_str_request_kind(uint8_t req_k);
void debug_request(request req);

request create_req_from_cstr(uint8_t kind, const char *c_str);

size_t size(request req);
uint8_t *flatten(request req);
request re_construct(uint8_t *header, uint8_t *raw_data);

void destroy_request(request *req);

request recv_req(int file_descriptor);
void send_req(int file_descriptor, request req);

void process_request(request req);

// #define PROTOCOL_IMPLEMENTATION
#ifdef PROTOCOL_IMPLEMENTATION
request create_req_from_cstr(uint8_t kind, const char *c_str) {
  request output;
  output.kind = kind;
  output.length = strlen(c_str);
  output.data = malloc(output.length);
  for (uint8_t i = 0; i < output.length; i++)
    output.data[i] = (uint8_t)c_str[i];
  return output;
}

size_t size(request req) { return req.length + 2; }

uint8_t *flatten(request req) {
  uint8_t *output = malloc(req.length + 2);
  output[0] = req.kind;
  output[1] = req.length;
  for (uint8_t i = 0; i < req.length; i++)
    output[i + 2] = req.data[i];
  return output;
}

request re_construct(uint8_t header[], uint8_t *raw) {
  request output;
  output.kind = header[0];
  output.length = header[1];
  output.data = malloc(output.length);
  for (uint8_t i = 0; i < output.length; i++)
    output.data[i] = raw[i];
  return output;
}

void destroy_request(request *req) {
  if (req->data)
    free(req->data);
}

char *debug_c_str_request_kind(uint8_t req_k) {
  switch (req_k) {
  case DISCONNECT:
    return ("DISCONNECT");
  case REQ_LOGIN:
    return ("REQ_LOGIN");
  case RSP_LOGIN:
    return ("RSP_LOGIN");
  case REQ_SIGN_UP:
    return ("REQ_SIGN_UP");
  case RSP_SIGN_UP:
    return ("RSP_SIGN_UP");
  case REQ_LOGOUT:
    return ("REQ_LOGOUT");
  case RSP_LOGOUT:
    return ("RSP_LOGOUT");
  case REQ_SEARCH_USER:
    return ("REQ_SEARCH_USER");
  case RSP_SEARCH_USER:
    return ("RSP_SEARCH_USER");
  case REQ_USER_INFO:
    return ("REQ_USER_INFO");
  case RSP_USER_INFO:
    return ("RSP_USER_INFO");
  case REQ_MESSAGE_SEND:
    return ("REQ_MESSAGE_SEND");
  case RSP_MESSAGE_SEND:
    return ("RSP_MESSAGE_SEND");
  case NTF_MESSAGE_RECV:
    return ("NTF_MESSAGE_RECV");
  case REQ_CHAT_ACCESS:
    return ("REQ_CHAT_ACCESS");
  case RSP_CHAT_ACCESS:
    return ("RSP_CHAT_ACCESS");
  case REQ_DM_ACCESS:
    return ("REQ_DM_ACCESS");
  case RSP_DM_ACCESS:
    return ("RSP_DM_ACCESS");
  case REQ_PING:
    return ("REQ_PING");
  case RSP_PONG:
    return ("RSP_PONG");
  case RSP_ERROR:
    return ("RSP_ERROR");

  default:
    return ("Unknown");
  }
}

void debug_request(request req) {
  char *tmp = debug_c_str_request_kind(req.kind);
  size_t sz = strlen(tmp);
  sz += strlen("Request(");
  sz += strlen(", uuu, [");
  sz += 3 * req.length - 1;
  sz += strlen("])0");
  char buf[sz];
  snprintf(buf, sizeof(buf), "Request(%s, %u, [", tmp, req.length);
  size_t j = strlen(buf);
  for (uint8_t i = 0; i < req.length; i++) {
    snprintf(buf + j, sizeof(buf) + j, "%x", req.data[i]);
    j = strlen(buf);
    if (i != req.length - 1) {
      snprintf(buf + j, sizeof(buf) + j, " ");
      j = strlen(buf);
    }
  }
  snprintf(buf + j, sizeof(buf) + j, "])");

  LOG_INFO("%s", buf);
}

request recv_req(int file_descriptor) {
  uint8_t req_header[2];
  ssize_t num = read(file_descriptor, &req_header, sizeof(req_header));
  if (num < 0) {
    printf("error recieving request : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (num == 0) {
    uint8_t *fd_data = malloc(sizeof(4));
    fd_data[0] = file_descriptor & 0xFF;
    fd_data[1] = (file_descriptor >> 8) & 0xFF;
    fd_data[2] = (file_descriptor >> 16) & 0xFF;
    fd_data[3] = (file_descriptor >> 24) & 0xFF;

    return (request){.kind = DISCONNECT, .length = 4, .data = fd_data};
  }
  uint8_t *req_payload = malloc(req_header[1]);
  if (read(file_descriptor, req_payload, req_header[1]) < 0) {
    printf("error recieving request : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  request output = re_construct(req_header, req_payload);
  return output;
}

void send_req(int file_descriptor, request req) {
  size_t sz = size(req);
  uint8_t *flat_req = flatten(req);

  if (write(file_descriptor, flat_req, sz) < 0) {
    printf("error sending to the server : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

// helper
bool is_num(char *c_str) {
  char *x = c_str;
  while (*x) {
    if (*x < '0' || *x > '9')
      return false;
    x++;
  }
  return true;
}
void process_request(request req) {
  LOG_INFO("Processing : ");
  debug_request(req);

  if (req.kind == DISCONNECT) {
    uint8_t *fd_data = req.data;
    int fd = (int)fd_data[0] | ((int)fd_data[1] << 8) |
             ((int)fd_data[2] << 16) | ((int)fd_data[3] << 24);
    LOG_INFO("disconnected file_descriptor = %d", fd);
    close(fd);
  }
}
#endif
#endif
