#include "storage.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  StorageLayer stl = create_file_based_storage("data/");
  uint8_t *body = malloc(10);
  for (int i = 0; i < 10; i++)
    body[i] = 'X';
  if (stl.store_message(stl.context, (message){"xyz", "hello", body, 10, 0})) {
    printf("successfully stored a message\n");
  } else {
    printf("error storing message\n");
    return 1;
  }
  if (stl.store_message(stl.context, (message){"hello", "xyz", body, 10, 1})) {
    printf("successfully stored a message\n");
  } else {
    printf("error storing message\n");
    return 1;
  }
  if (stl.store_message(stl.context, (message){"xyz", "hello", body, 10, 2})) {
    printf("successfully stored a message\n");
  } else {
    printf("error storing message\n");
    return 1;
  }
  message_s msgs = stl.load_messages(stl.context, "hello", "xyz");

  printf("have total %zu messages from xyz to hello\n", msgs.size);
  for (size_t i = 0; i < msgs.size; i++) {
    printf("message %zu : ", msgs.msgs[i].msg_id);
    for (size_t j = 0; j < msgs.msgs[i].size; j++)
      printf("%c", msgs.msgs[i].body[j]);
    printf("\n");
  }

  stl.delete_message(stl.context, 0);
  printf("deleted mesage id = 0\n");

  msgs = stl.load_messages(stl.context, "hello", "xyz");

  printf("have total %zu messages from xyz to hello\n", msgs.size);
  for (size_t i = 0; i < msgs.size; i++) {
    printf("message %zu : ", msgs.msgs[i].msg_id);
    for (size_t j = 0; j < msgs.msgs[i].size; j++)
      printf("%c", msgs.msgs[i].body[j]);
    printf("\n");
  }
  return 0;
}
