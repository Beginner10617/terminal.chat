#ifndef HASH_MAP
#define HASH_MAP
#include "stddef.h"
#include <stdbool.h>
typedef struct Entry {
  int key;
  void *value;
  struct Entry *next;
} Entry;

typedef struct {
  Entry **buckets;
  size_t bucket_count;
} HashMap;
// constructor
HashMap *hashmap_create(size_t bucket_count);
// re-hash
void hashmap_resize(HashMap *map, size_t new_bucket_count);
// compute load factor
float load_factor(HashMap *map);
// adding elements
void hashmap_put(HashMap *map, int key, void *value);
// accessing elements
void *hashmap_get(HashMap *map, int key);
bool hashmap_contains(HashMap *map, int key);
// deletion
void hashmap_remove(HashMap *map, int key);
// destructor
void hashmap_destroy(HashMap *map);
#endif
