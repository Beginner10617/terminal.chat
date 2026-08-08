#ifndef STORAGE_H
#define STORAGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MAX_PUBLIC_KEY_SIZE 1024
#define MAX_PASSWORD_HASH_SIZE 256
#define MAX_SALT_SIZE 8

typedef struct {
  const char *username;
  const uint8_t public_key[MAX_PUBLIC_KEY_SIZE],
      password_hash[MAX_PASSWORD_HASH_SIZE], salt[8];
} user_cred;

typedef struct {
  const char *from_username, *to_username;
  uint8_t *body;
  size_t size, table_index;
} message;

typedef struct {
  bool (*create_user)(user_cred);

  bool (*authenticate)(const char *username, const uint8_t *password);

  bool (*store_message)(message);

  bool (*load_messages_to)(const char *to_username);

  bool (*delete_message)(message);
} StorageLayer;

#endif
