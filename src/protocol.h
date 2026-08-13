#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <openssl/ssl.h>
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
  RSP_ERROR, // send to client
  REQ_ERROR, // for internal logging
} REQUEST_RESPONSE_KIND;

typedef struct {
  uint8_t kind, length, *data;
  int file_descriptor; // to be used to give response
  SSL *ssl;
} request;

char *debug_c_str_request_kind(uint8_t req_k);
void debug_request(request req);

request create_req_from_cstr(uint8_t kind, const char *c_str);

size_t size(request req);
uint8_t *flatten(request req);
request re_construct(uint8_t *header, uint8_t *raw_data);

void destroy_request(request *req);

request recv_req(SSL *ssl);
void send_req(SSL *ssl, request req);

void process_request(request req);

typedef struct {
  request *requests;
  size_t size, cap, front, rear;
} request_queue;
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
  case REQ_ERROR:
    return ("REQ_ERROR");

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

bool read_exact(SSL *ssl, void *buf, size_t size) {
  size_t recv = 0;
  while (recv < size) {
    int num = SSL_read(ssl, (uint8_t *)buf + recv, size - recv);
    if (num <= 0) {
      int err = SSL_get_error(ssl, num);

      switch (err) {
      case SSL_ERROR_ZERO_RETURN:
        // Clean TLS shutdown (close_notify)
        return false;

      case SSL_ERROR_SYSCALL:
        LOG_ERROR("underlying socket error / unexpected EOF");
        return false;

      default:
        // Actual TLS error
        ERR_print_errors_fp(stderr);
        return false;
      }
    }
    recv += num;
  }
  return true;
}

request recv_req(SSL *ssl) {
  uint8_t req_header[2];
  if (!read_exact(ssl, req_header, sizeof(req_header))) {
    if (errno == 0)
      return (request){.kind = DISCONNECT, .length = 0, .data = NULL};
    LOG_ERROR("error recieving request : %s", strerror(errno));
    return (request){.kind = REQ_ERROR, .length = 0, .data = NULL};
  }
  uint8_t *req_payload = malloc(req_header[1]);
  if (req_payload == NULL) {
    LOG_ERROR("error allocating for request payload");
    return (request){.kind = REQ_ERROR, .length = 0, .data = NULL};
  }
  if (!read_exact(ssl, req_payload, req_header[1])) {
    free(req_payload);
    LOG_ERROR("error recieving request : %s\n", strerror(errno));
    return (request){.kind = REQ_ERROR, .length = 0, .data = NULL};
  }
  request output = re_construct(req_header, req_payload);
  return output;
}

void send_req(SSL *ssl, request req) {
  size_t sz = size(req);
  uint8_t *flat_req = flatten(req);

  if (SSL_write(ssl, flat_req, sz) < 0) {
    LOG_ERROR("error sending to the server : %s\n", strerror(errno));
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
    int fd = req.file_descriptor;
    SSL *ssl = req.ssl;
    LOG_INFO("disconnected file_descriptor = %d", fd);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);
  }
}

void init_request_queue(request_queue *que) {
  que->requests = malloc(sizeof(request));
  que->cap = 1;
  que->size = 0;
  que->front = 0;
  que->rear = 0;
}

request front_request_queue(request_queue que) {
  return que.requests[que.front];
}

void push_request_queue(request_queue *que, request req) {
  if (que->size == que->cap) {
    size_t new_cap = que->cap * 2;
    request *tmp = malloc(sizeof(request) * new_cap);
    if (tmp == NULL) {
      LOG_ERROR("unable to allocate space to expand request queue");
      return;
    }
    for (size_t i = 0; i < que->size; i++) {
      tmp[i] = que->requests[(que->front + i) % que->cap];
    }
    tmp[que->size] = req;
    que->size++;
    que->cap = new_cap;
    que->front = 0;
    que->rear = que->size;
    free(que->requests);
    que->requests = tmp;
    return;
  }
  que->requests[que->rear] = req;
  que->rear = (que->rear + 1) % que->cap;
  que->size++;
}

void pop_request_queue(request_queue *que) {
  if (que->size == 0)
    return;
  free(que->requests[que->front].data);
  que->front = (que->front + 1) % que->cap;
  que->size--;
}

void debug_print_queue(request_queue que) {
  LOG_INFO("Queue([");
  for (size_t i = 0; i < que.size; i++) {
    debug_request(que.requests[(que.front + i) % que.cap]);
  }
  LOG_INFO("])");
}

bool is_empty_request_queue(request_queue que) { return que.size == 0; }

#endif
#endif
