#include "storage.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// context = char *paths[2] = {path_user_table, path_messages_table}
bool file_based_create_user(void *ctx, user_cred uc) {
  const char *filepath = ((const char **)ctx)[0];
  FILE *file = fopen(filepath, "ab");
  if (file == NULL)
    return false;
  size_t size_of_uc = sizeof(uc) + strlen(uc.username);
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
      return false;
    char tmp_c;
    for (size_t i = 0; i < size_of_username; i++) {
      fread(&tmp_c, sizeof(char), 1, file);
      if (username[i] != tmp_c)
        return false;
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
