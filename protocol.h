#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>
#include <stdio.h>
#define MAX_LENGTH 512

typedef enum {
  LOGIN,
  MESSAGE_SEND,
  CHAT_ACCESS,
} REQUEST_KIND;

typedef struct {
  uint8_t kind, length;
} request_header;

void debug_print_request(request req);
void debug_print_request_kind(uint8_t req_k);
request create_req_from_cstr(uint8_t kind, const char *c_str);

// #define PROTOCOL_IMPLEMENTATION
#ifdef PROTOCOL_IMPLEMENTATION
request create_req(uint8_t kind, const char *c_str) {
  request output;
  output.kind = kind;
  size_t i = 0;
  while (i < MAX_LENGTH && c_str[i]) {
    output.data[i] = (uint8_t)c_str[i];
    i++;
  }
  if (i == MAX_LENGTH && !c_str[i]) {
#ifdef DEBUG
    printf("warning : the request data should be of length at most %u and "
           "shold be null terminated\n",
           MAX_LENGTH);
#endif
  }
  output.data[i] = 0;
  return output;
}

void debug_print_request(request req) {
  printf("Request kind : ");
  debug_print_request_kind(req.kind);
  printf("\nData :\n");
  size_t i = 0;
  while (i < MAX_LENGTH && req.data[i]) {
    printf("%x ", req.data[i]);
    i++;
  }
  printf("\n");
}

void debug_print_request_kind(uint8_t req_k) {
  switch (req_k) {
  case LOGIN:
    printf("LOGIN");
    break;
  case MESSAGE_SEND:
    printf("MESSAGE_SEND");
    break;
  case CHAT_ACCESS:
    printf("CHAT_ACCESS");
    break;
  default:
    printf("Unknown");
    break;
  }
}
#endif
#endif
