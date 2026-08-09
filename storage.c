#include "storage.h"
#include <stdio.h>

bool file_based_create_user(void *ctx, user_cred uc) {
  const char *filepath = (const char *)ctx;
  FILE *file = fopen(filepath, "ab");
  if (file == NULL)
    return false;
}
