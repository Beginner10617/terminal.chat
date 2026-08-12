#include "storage.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_message_s(message_s *msgs) {
  msgs->cap = 1;
  msgs->size = 0;
  msgs->msgs = malloc(sizeof(message));
  if (msgs->msgs == NULL) {
    LOG_ERROR("unable to allocate space for initlising message_s");
    return;
  }
}

void append_message_s(message_s *msgs, message msg) {
  if (msgs->size == msgs->cap) {
    message *tmp;
    msgs->cap *= 2;
    tmp = realloc(msgs->msgs, msgs->cap);
    if (tmp == NULL) {
      LOG_ERROR("unable to allocate space for new message");
      return;
    }
    msgs->msgs = tmp;
  }
  msgs->msgs[msgs->size] = msg;
  msgs->size++;
  return;
}

// context = char *paths[2] = {path_user_table, path_messages_table}
bool file_based_create_user(void *ctx, user_cred uc) {
  char *filepath = ((char **)ctx)[0];
  FILE *file = fopen(filepath, "ab");
  if (file == NULL)
    return false;
  size_t size_of_uc = MAX_PASSWORD_HASH_SIZE + MAX_PUBLIC_KEY_SIZE +
                      MAX_SALT_SIZE + strlen(uc.username) + 1;
  fwrite(&size_of_uc, sizeof(size_of_uc), 1, file);
  fwrite(uc.username, 1, strlen(uc.username) + 1, file);
  fwrite(uc.public_key, sizeof(uint8_t), MAX_PUBLIC_KEY_SIZE, file);
  fwrite(uc.password_hash, sizeof(uint8_t), MAX_PASSWORD_HASH_SIZE, file);
  fwrite(uc.salt, sizeof(uint8_t), MAX_SALT_SIZE, file);
  fclose(file);
  return true;
}

bool file_based_authenticate(void *ctx, const char *username,
                             const uint8_t *password_hash) {
  // we are assuming the username exists in the table

  const char *filepath = ((const char **)ctx)[0];
  FILE *file = fopen(filepath, "rb");
  if (file == NULL)
    return false;
  size_t size_of_curr;
  bool reading = true;
  while (reading) {
    fread(&size_of_curr, sizeof(size_t), 1, file);
    size_t size_of_username = size_of_curr - MAX_PUBLIC_KEY_SIZE -
                              MAX_PASSWORD_HASH_SIZE - MAX_SALT_SIZE;
    if (size_of_username != strlen(username) + 1)
      continue;
    char tmp_c;
    size_t i;
    for (i = 0; i < size_of_username; i++) {
      fread(&tmp_c, sizeof(char), 1, file);
      if (username[i] != tmp_c) {
        i++;
        break;
      }
    }
    if (i != size_of_username) {
      fseek(file, size_of_curr - i, SEEK_CUR);
      continue;
    }
    reading = false;
  }
  fseek(file, MAX_PUBLIC_KEY_SIZE, SEEK_CUR);
  uint8_t pwd_hash[MAX_PASSWORD_HASH_SIZE];
  fread(pwd_hash, sizeof(uint8_t), MAX_PASSWORD_HASH_SIZE, file);
  fclose(file);
  for (size_t i = 0; i < MAX_PASSWORD_HASH_SIZE; i++) {
    if (pwd_hash[i] != password_hash[i])
      return false;
  }
  return true;
}

user_cred file_based_find_user(void *ctx, const char *username) {
  const char *filepath = ((const char **)ctx)[0];
  FILE *file = fopen(filepath, "rb");
  if (file == NULL) {
    return (user_cred){.username = "error"};
  }
  size_t size_of_curr;
  bool reading = true;
  while (reading) {
    size_t n = fread(&size_of_curr, sizeof(size_t), 1, file);
    if (n == 0)
      return (user_cred){.username = "not found"};
    size_t size_of_username = size_of_curr - MAX_PUBLIC_KEY_SIZE -
                              MAX_PASSWORD_HASH_SIZE - MAX_SALT_SIZE;
    // printf("size_of_username : %zu\n", size_of_username);
    // printf("size_of_curr : %zu\n", size_of_curr);
    if (size_of_username != strlen(username) + 1) {
      fseek(file, size_of_curr - sizeof(size_of_username), SEEK_CUR);
      continue;
    }
    char tmp_c;
    size_t i;
    for (i = 0; i < size_of_username; i++) {
      size_t n = fread(&tmp_c, sizeof(char), 1, file);
      if (n == 0)
        return (user_cred){.username = "not found"};
      if (username[i] != tmp_c) {
        i++;
        break;
      }
    }
    if (i != size_of_username) {
      fseek(file, size_of_curr - i, SEEK_CUR);
      continue;
    }
    reading = false;
  }
  user_cred out = {.username = username};
  fread(out.public_key, sizeof(uint8_t), MAX_PUBLIC_KEY_SIZE, file);
  fread(out.password_hash, sizeof(uint8_t), MAX_PASSWORD_HASH_SIZE, file);
  fread(out.salt, sizeof(uint8_t), MAX_SALT_SIZE, file);
  fclose(file);
  return out;
}

void file_based_delete_user(void *ctx, const char *username) {
  const char *filepath = ((const char **)ctx)[0];
  FILE *file = fopen(filepath, "rb+");
  if (file == NULL) {
    return;
  }
  size_t size_of_curr;
  bool reading = true;
  while (reading) {
    fread(&size_of_curr, sizeof(size_t), 1, file);
    size_t size_of_username = size_of_curr - MAX_PUBLIC_KEY_SIZE -
                              MAX_PASSWORD_HASH_SIZE - MAX_SALT_SIZE;
    if (size_of_username != strlen(username) + 1)
      continue;
    char tmp_c;
    size_t i;
    for (i = 0; i < size_of_username; i++) {
      size_t n = fread(&tmp_c, sizeof(char), 1, file);
      if (username[i] != tmp_c) {
        i++;
        break;
      }
    }
    if (i != size_of_username) {
      fseek(file, size_of_curr - i, SEEK_CUR);
      continue;
    }
    reading = false;
  }
  fseek(file, -(strlen(username) + 1), SEEK_CUR);
  char empty[strlen(username) + 1];
  memset(empty, ' ', sizeof(empty));
  empty[strlen(username)] = 0;
  fwrite(empty, sizeof(char), sizeof(empty), file);
  fclose(file);
  return;
}

bool file_based_store_message(void *ctx, message msg) {
  const char *filepath = ((const char **)ctx)[1];
  FILE *file = fopen(filepath, "ab");
  if (file == NULL)
    return false;
  size_t size = strlen(msg.from_username) + strlen(msg.to_username) + 2 +
                msg.size + sizeof(msg.msg_id);
  fwrite(&size, sizeof(size), 1, file);
  fwrite(msg.from_username, sizeof(char), strlen(msg.from_username) + 1, file);
  fwrite(msg.to_username, sizeof(char), strlen(msg.to_username) + 1, file);
  fwrite(msg.body, 1, msg.size, file);
  fwrite(&msg.msg_id, sizeof(msg.msg_id), 1, file);
  fclose(file);
  return true;
}

message_s file_based_load_messages(void *ctx, const char *to_username,
                                   const char *from_username) {
  message_s out = {NULL, 0, 1};
  const char *filepath = ((char **)ctx)[1];
  FILE *file = fopen(filepath, "rb");
  if (file == NULL)
    return out;
  init_message_s(&out);
  size_t size_of_curr;
  while (true) {
    size_t n = fread(&size_of_curr, sizeof(size_of_curr), 1, file);
    if (n == 0) {
      break;
    }
    // compare the strings (null terminated)
    // if not match, jump with appropriate offset
    // if match, construct and append the message object to out
    // IMP : verify n != 0 on all fread() calls
  }
  return out;
}

StorageLayer create_file_based_storage(char *dirpath) {
  const char *user_table_name = "user";
  char **context = malloc(sizeof(char *) * 2);
  context[0] =
      malloc(sizeof(char) * (strlen(dirpath) + strlen(user_table_name) + 1));
  strcpy(context[0], dirpath);
  strcat(context[0], user_table_name);
  return (StorageLayer){.context = context,
                        .create_user = file_based_create_user,
                        .authenticate = file_based_authenticate,
                        .find_user = file_based_find_user,
                        .delete_user = file_based_delete_user,
                        .store_message = file_based_store_message,
                        .load_messages = file_based_load_messages};
}
