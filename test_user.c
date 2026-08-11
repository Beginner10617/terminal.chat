#include "storage.h"
#include <stdio.h>

int main() {
  StorageLayer stl = create_file_based_storage("data/");
  printf("context[0] after creation : %s\n", ((char **)stl.context)[0]);
  char *f = ((char **)stl.context)[0];
  printf("f : %s\n", f);
  if (stl.create_user(stl.context, (user_cred){"hello", {0}, {0}, {0}})) {
    printf("succesfully created user!\n");
  } else {
    printf("error creating user\n");
  }
  return 0;
}
