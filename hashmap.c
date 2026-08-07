#include "hashmap.h"
#include "stdio.h"
#include "string.h"
#include <stddef.h>
#include <stdlib.h>
#define ERROR "\x1b[31m"
#define WARNING "\033[33m"
#define COLOR_RESET "\x1b[0m"

HashMap *hashmap_create(size_t bucket_count) {
  HashMap *out = malloc(sizeof(HashMap));
  if (out == NULL) {
    printf(ERROR "Unable to allocate space for hashmap\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }
  out->bucket_count = bucket_count;
  out->buckets = malloc(sizeof(Entry *) * bucket_count);
  if (out->buckets == NULL) {
    printf(ERROR "Unable to allocate bucket for hash-map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < bucket_count; i++)
    out->buckets[i] = NULL;
  return out;
}

void hashmap_resize(HashMap *map, size_t new_bucket_count) {
  if (map == NULL) {
    printf(ERROR "NULL pointer passed as map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }
  if (map->bucket_count > new_bucket_count) {
    printf(WARNING
           "Reducing bucket size might introduce collisions\n" COLOR_RESET);
  }

  Entry **old_buckets = map->buckets;
  size_t old_bucket_count = map->bucket_count;

  Entry **new_buckets = calloc(new_bucket_count, sizeof(Entry *));

  if (new_buckets == NULL) {
    printf(ERROR "Unable to allocate resized hash-map buckets\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  map->buckets = new_buckets;
  map->bucket_count = new_bucket_count;

  for (size_t i = 0; i < old_bucket_count; i++) {

    Entry *curr = old_buckets[i];

    while (curr != NULL) {

      Entry *next = curr->next;

      size_t bucket_index = curr->key % new_bucket_count;

      curr->next = new_buckets[bucket_index];
      new_buckets[bucket_index] = curr;

      curr = next;
    }
  }

  free(old_buckets);
}

float load_factor(HashMap *map) {
  if (map == NULL) {
    printf(ERROR "NULL pointer passed as map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }
  size_t num_of_entries = 0;
  for (size_t i = 0; i < map->bucket_count; i++) {
    Entry *curr = map->buckets[i];
    while (curr != NULL) {
      Entry *next = curr->next;
      num_of_entries++;
      curr = next;
    }
  }
  return (float)num_of_entries / (float)map->bucket_count;
}

void hashmap_put(HashMap *map, int key, void *value) {
  if (map == NULL) {
    printf(ERROR "NULL pointer passed as map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  size_t bucket_index = key % map->bucket_count;
  Entry *curr = map->buckets[bucket_index];
  while (curr != NULL) {
    if (curr->key == key) {
      free(curr->value);
      curr->value = value;
      return;
    }
    curr = curr->next;
  }
  Entry *new_entry = malloc(sizeof(Entry));
  new_entry->value = value;
  new_entry->key = key;
  new_entry->next = map->buckets[bucket_index];
  map->buckets[bucket_index] = new_entry;
}

void *hashmap_get(HashMap *map, int key) {
  if (map == NULL) {
    printf(ERROR "NULL pointer passed as map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  size_t bucket_index = key % map->bucket_count;
  Entry *curr = map->buckets[bucket_index];
  while (curr != NULL) {
    if (curr->key == key)
      return curr->value;
    curr = curr->next;
  }
  return NULL;
}
bool hashmap_contains(HashMap *map, int key) {
  if (map == NULL) {
    printf(ERROR "NULL pointer passed as map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  size_t bucket_index = key % map->bucket_count;
  Entry *curr = map->buckets[bucket_index];
  while (curr != NULL) {
    if (curr->key == key)
      return true;
    curr = curr->next;
  }
  return false;
}

void hashmap_remove(HashMap *map, int key) {
  if (map == NULL) {
    printf(ERROR "NULL pointer passed as map\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  size_t bucket_index = key % map->bucket_count;
  Entry *curr = map->buckets[bucket_index];
  Entry *prev = NULL;
  while (curr != NULL) {
    if (curr->key == key == 0) {
      if (prev)
        prev->next = curr->next;
      else
        map->buckets[bucket_index] = curr->next;
      if (curr->value)
        free(curr->value);
      free(curr);
      // HashMap own the value
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}
// helper
void freeEntry(Entry *entry) {
  if (entry == NULL)
    return;
  freeEntry(entry->next);
  if (entry->value)
    free(entry->value);
  free(entry);
}

void hashmap_destroy(HashMap *map) {
  for (size_t i = 0; i < map->bucket_count; i++)
    freeEntry(map->buckets[i]);
  free(map->buckets);
  free(map);
}
