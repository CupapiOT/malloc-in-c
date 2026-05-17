#define _GNU_SOURCE // We need this for sbrk() to be declared.
#include "halloc.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define META_SIZE sizeof(struct heapchunk)
// #define HEAP_SIZE 4096

struct heapchunk {
  struct heapchunk *next;
  size_t size;
  bool free;
};
typedef struct heapchunk heapchunk;

// struct heap {
//   void *first_chunk;
//   size_t available;
// };
//
// void init_heap() {
//   global_heap_base = sbrk(HEAP_SIZE);
//   global_heap_base->available = 4096;
//   global_heap_base->first_chunk = sbrk(0);
// }

heapchunk *global_heap_base = NULL;

void inspect_chunk(heapchunk *chunk) {
  printf("chunk = %p\n", (void *)chunk);
  printf("chunk->next = %p\n", (void *)chunk->next);
  printf("chunk->size = %zu\n", chunk->size);
  printf("chunk->free = %s\n", chunk->free ? "true" : "false");
  printf("bytes = ");
  unsigned char *bytes = (unsigned char *)(chunk + 1);
  for (size_t i = 0; i < chunk->size; i++) {
    printf("%c", bytes[i]);
  }
  if (bytes[chunk->size - 2] != '\n') {
    printf("\n");
  }
}

void inspect_all_chunks() {
  heapchunk *curr_chunk = global_heap_base;
  size_t counter = 0;
  printf("\n============================");
  printf("\n===== INSPECTION START =====");
  printf("\n============================\n\n");
  while (curr_chunk != NULL) {
    printf("========= Chunk %02zu =========\n", counter++);
    inspect_chunk(curr_chunk);
    curr_chunk = curr_chunk->next;
  }
  printf("\n============================");
  printf("\n=====  INSPECTION END  =====");
  printf("\n============================\n");
}

heapchunk *find_free_chunk(size_t size) {
  heapchunk *curr_chunk = global_heap_base;
  while (curr_chunk != NULL) {
    if (curr_chunk->free == true && curr_chunk->size >= size) {
      // curr_chunk->size = size;
      curr_chunk->free = false;
      return curr_chunk;
    }
    curr_chunk = curr_chunk->next;
  };
  return NULL;
}

heapchunk *create_chunk(size_t size) {
  heapchunk *new_chunk = sbrk(META_SIZE + size);
  if (new_chunk == (void *)-1)
    return NULL;
  new_chunk->size = size;
  new_chunk->free = false;
  new_chunk->next = NULL;
  return new_chunk;
}

void append_chunk(heapchunk *chunk) {
  heapchunk *curr_chunk = global_heap_base;
  while (curr_chunk->next != NULL) {
    printf("curr-chunk-next %p\n", (void *)curr_chunk->next);
    curr_chunk = curr_chunk->next;
  }
  curr_chunk->next = chunk;
}

void *heapmalloc(size_t size) {
  if (size == 0)
    return NULL;

  printf("\n\n\n===== BEFORE =====\n");
  inspect_all_chunks();
  void *ptr;
  if (global_heap_base != NULL) {
    heapchunk *free_chunk = find_free_chunk(size);
    if (free_chunk == NULL) {
      free_chunk = create_chunk(size);
      if (free_chunk == NULL)
        return NULL;
      append_chunk(free_chunk);
    }
    ptr = free_chunk + 1;
  } else {
    void *original = sbrk(0);
    global_heap_base = sbrk(META_SIZE + size);
    if (global_heap_base == (void *)-1) {
      return NULL;
    }
    global_heap_base->size = size;
    global_heap_base->free = false;
    global_heap_base->next = NULL;
    assert(original == global_heap_base); // Not thread safe
    ptr = global_heap_base + 1;
  }
  printf("\n\n\n===== AFTER  =====\n");
  inspect_all_chunks();
  return ptr;
}

void heapfree(void *ptr) {
  heapchunk *chunk = (heapchunk *)ptr - 1;
  chunk->free = true;
  unsigned char garbage = 0xFE;
  memset(ptr, garbage, chunk->size);
}
