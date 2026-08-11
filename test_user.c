#include "storage.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  StorageLayer stl = create_file_based_storage("data/");
  printf("context[0] after creation : %s\n", ((char **)stl.context)[0]);
  if (stl.create_user(stl.context, (user_cred){"hello", {1}, {1}, {1}})) {
    printf("succesfully created user hello!\n");
  } else {
    printf("error creating user hello\n");
    return 1;
  }
  user_cred uc = stl.find_user(stl.context, "hello");
  printf("username : %s\n", uc.username);
  if (strncmp(uc.username, "hello", 5) == 0) {
    printf("found the user hello!\n");
  } else {
    printf("unable to find user hello\n");
    return 1;
  }
  uint8_t hash[MAX_PASSWORD_HASH_SIZE] = {0};
  printf("authenticating with hash = {0,...,0}\n");
  if (stl.authenticate(stl.context, "hello", hash)) {
    printf("authentication succesful!\n");
    return 1;
  } else {
    printf("authentication failed\n");
  }
  hash[0] = 1;
  printf("authenticating with hash = {1, 0,...,0}\n");
  if (stl.authenticate(stl.context, "hello", hash)) {
    printf("authentication succesful!\n");
  } else {
    printf("authentication failed\n");
    return 1;
  }

  printf("deleting user hello...\n");
  stl.delete_user(stl.context, "hello");
  uc = stl.find_user(stl.context, "hello");
  printf("username : %s\n", uc.username);
  if (strncmp(uc.username, "hello", 5) == 0) {
    printf("found the user hello!");
    return 1;
  } else {
    printf("unable to find user hello\n");
  }

  return 0;
}
