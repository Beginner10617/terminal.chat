#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>

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
  NTF_MESSAGE_RECV,
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

void debug_print_request_kind(uint8_t req_k);
void debug_print_request(request req);

request create_req_from_cstr(uint8_t kind, const char *c_str);

size_t size(request req);
uint8_t *flatten(request req);
request re_construct(uint8_t *header, uint8_t *raw_data);

void destroy_request(request *req);

request recv_req(int file_descriptor);
void send_req(int file_descriptor, request req);

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

void debug_print_request_kind(uint8_t req_k) {
  switch (req_k) {
  case REQ_LOGIN:
    printf("LOGIN");
    break;
  case REQ_MESSAGE_SEND:
    printf("MESSAGE_SEND");
    break;
  case REQ_CHAT_ACCESS:
    printf("CHAT_ACCESS");
    break;
  default:
    printf("Unknown");
    break;
  }
}

void debug_print_request(request req) {
  printf("Kind : ");
  debug_print_request_kind(req.kind);
  printf("\nLength : %u\nData :\n", req.length);
  for (uint8_t i = 0; i < req.length; i++)
    printf("%x ", req.data[i]);
  printf("\n");
}

request recv_req(int file_descriptor) {
  uint8_t req_header[2];
  ssize_t num = read(file_descriptor, &req_header, sizeof(req_header));
  if (num < 0) {
    printf("error recieving request : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (num == 0) {
    return (request){.kind = DISCONNECT, .length = 0};
  }
  printf("Header : %x %x\n", req_header[0], req_header[1]);
  uint8_t *req_payload = malloc(req_header[1]);
  if (read(file_descriptor, req_payload, req_header[1]) < 0) {
    printf("error recieving request : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  printf("Read the data!\n");
  request output = re_construct(req_header, req_payload);
  printf("Re-constructed!\n");
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
#endif
#endif
