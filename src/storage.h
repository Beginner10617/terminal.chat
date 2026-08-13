#ifndef STORAGE_H
#define STORAGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define LOG_INFO(format, ...)                                                  \
  fprintf(stderr, "[INFO] " format "\n", ##__VA_ARGS__)
#define LOG_ERROR(format, ...)                                                 \
  fprintf(stderr, "[ERROR] " format "\n", ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                                 \
  fprintf(stderr, "[DEBUG] " format "\n", ##__VA_ARGS__)
#define MAX_PUBLIC_KEY_SIZE 1024
#define MAX_PASSWORD_HASH_SIZE 256
#define MAX_SALT_SIZE 8

typedef struct {
  const char *username;
  uint8_t public_key[MAX_PUBLIC_KEY_SIZE],
      password_hash[MAX_PASSWORD_HASH_SIZE], salt[MAX_SALT_SIZE];
} user_cred;

typedef struct {
  const char *from_username, *to_username;
  uint8_t *body;
  size_t size, msg_id;
} message;

typedef struct {
  message *msgs;
  size_t size, cap;
} message_s;
void init_message_s(message_s *);
void append_message_s(message_s *, message);

typedef struct {
  void *context;

  bool (*create_user)(void *ctx, user_cred);

  bool (*authenticate)(void *ctx, const char *username,
                       const uint8_t *password_hash);

  user_cred (*find_user)(void *ctx, const char *username);

  void (*delete_user)(void *ctx, const char *username);

  bool (*store_message)(void *ctx, message);

  message_s (*load_messages)(void *ctx, const char *to_username,
                             const char *from_username);

  void (*delete_message)(void *ctx, size_t msg_id);
} StorageLayer;

StorageLayer create_file_based_storage(char *dirpath);

#endif
