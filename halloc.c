#define _GNU_SOURCE // We need this for sbrk() to be declared.
#include "halloc.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALIGN_BYTES _Alignof(max_align_t) // C11 or above.
#define META_SIZE sizeof(struct heapchunk_meta)
#define PADDING_BYTE 0xAD
#define FREE_BYTE 0xFE

/*
 * NOTE: All asserts are replaced with `abort()`s or `return NULL`s in
 * ./custom-malloc.c.
 */

// #define HEAP_SIZE 4096
// struct heap {
//   void *first_chunk;
//   size_t available;
// };
//
// void init_heap() {
//   global_heap_base = sbrk(HEAP_SIZE);
//   global_heap_base->available = HEAP_SIZE;
//   global_heap_base->first_chunk = global_heap_base;
// }

struct heapchunk_meta {
  struct heapchunk_meta *next;
  size_t size;
  bool free; // Replaced with unsigned char in prod version.
  unsigned char padding;
  unsigned short magic;
};
typedef struct heapchunk_meta heapchunk_meta;

static heapchunk_meta *global_heap_base = NULL;

static void inspect_chunk(heapchunk_meta *chunk) {
  if (chunk == NULL) {
    printf("Chunk is NULL: %p\n", (void *)chunk);
    return;
  }
  printf("chunk       = %p\n", (void *)chunk);
  printf("ptr         = %p\n", (void *)(chunk + 1));
  printf("chunk->next = %p\n", (void *)chunk->next);
  printf("chunk->size = %zu\n", chunk->size);
  printf("chunk->free = %s\n", chunk->free ? "true" : "false");
  printf("chunk->padding = %hu\n", chunk->padding);
  printf("bytes = ");
  unsigned char *bytes = (unsigned char *)(chunk + 1);
  size_t bytes_to_print = chunk->size <= 32 ? chunk->size : 32;
  for (size_t i = 0; i < bytes_to_print; i++) {
    if (isalnum(bytes[i]))
      printf("%c ", bytes[i]);
    else
      printf("0x%02x ", bytes[i]);
  }
  if (bytes[chunk->size - 2] != '\n')
    printf("\n");
}

void inspect_all_chunks() {
  heapchunk_meta *curr_chunk = global_heap_base;
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

static size_t get_heap_size() {
  size_t curr_addr = (size_t)sbrk(0);
  size_t first_addr =
      (size_t)((char *)global_heap_base - global_heap_base->padding);
  return curr_addr - first_addr;
}

static void byte_to_char(unsigned char byte, char buf[3]) {
  buf[2] = '\0';

  unsigned char nibble = byte & 0x0F;
  if (nibble <= 0x9)
    buf[1] = nibble + 48;
  else if (nibble >= 0xA && nibble <= 0xF)
    buf[1] = nibble + 55;
  nibble = (byte >> 4) & 0xF;
  if (nibble <= 0x9)
    buf[0] = nibble + 48;
  else if (nibble >= 0xA && nibble <= 0xF)
    buf[0] = nibble + 55;
}

void heaphexdump() {
  if (global_heap_base == NULL) {
    printf("Nothing to dump.\n");
  }

  const size_t BYTES_PER_ROW = 16;
  size_t total_heap_size = get_heap_size();
  size_t total_rows = total_heap_size / BYTES_PER_ROW;
  if (total_heap_size % BYTES_PER_ROW != 0)
    total_rows++;

  char *base = (char *)global_heap_base - global_heap_base->padding;
  printf("TOTAL_HEAP_SIZE = %zu\n", total_heap_size);
  printf(
      "address           00 01 02 03  04 05 06 07  08 09 10 11  12 13 14 15\n");
  printf(
      "--------------------------------------------------------------------\n");
  for (size_t curr_row = 0; curr_row < total_rows; curr_row++) {
    char *curr_pos = base + (BYTES_PER_ROW * curr_row);
    char byte0[3];
    byte_to_char(curr_pos[0], byte0);
    char byte1[3];
    byte_to_char(curr_pos[1], byte1);
    char byte2[3];
    byte_to_char(curr_pos[2], byte2);
    char byte3[3];
    byte_to_char(curr_pos[3], byte3);
    char byte4[3];
    byte_to_char(curr_pos[4], byte4);
    char byte5[3];
    byte_to_char(curr_pos[5], byte5);
    char byte6[3];
    byte_to_char(curr_pos[6], byte6);
    char byte7[3];
    byte_to_char(curr_pos[7], byte7);
    char byte8[3];
    byte_to_char(curr_pos[8], byte8);
    char byte9[3];
    byte_to_char(curr_pos[9], byte9);
    char byte10[3];
    byte_to_char(curr_pos[10], byte10);
    char byte11[3];
    byte_to_char(curr_pos[11], byte11);
    char byte12[3];
    byte_to_char(curr_pos[12], byte12);
    char byte13[3];
    byte_to_char(curr_pos[13], byte13);
    char byte14[3];
    byte_to_char(curr_pos[14], byte14);
    char byte15[3];
    byte_to_char(curr_pos[15], byte15);
    printf("%p    %s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s\n",
           (void *)curr_pos, byte0, byte1, byte2, byte3, byte4, byte5, byte6,
           byte7, byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15);
  }
}

static heapchunk_meta *find_free_chunk(heapchunk_meta **last_chunk,
                                       size_t size) {
  heapchunk_meta *curr_chunk = global_heap_base;
  while (curr_chunk != NULL &&
         !(curr_chunk->free == true && curr_chunk->size >= size)) {
    *last_chunk = curr_chunk;
    curr_chunk = curr_chunk->next;
  };
  return curr_chunk;
}

static heapchunk_meta *create_chunk(size_t size) {
  void *curr = sbrk(0);
  if (curr == (void *)-1)
    return NULL;

  size_t ptr_addr = (size_t)curr + META_SIZE;
  size_t padding =
      (ALIGN_BYTES - (ptr_addr & (ALIGN_BYTES - 1))) & (ALIGN_BYTES - 1);
  size_t total_size = META_SIZE + padding + size;
  void *request = sbrk(total_size);

  assert(curr == request);             // Not thread safe.
  memset(curr, PADDING_BYTE, padding); // REMOVE IN NON-DEBUG MODE

  heapchunk_meta *new_chunk = (void *)((char *)curr + padding);

  new_chunk->next = NULL;
  new_chunk->size = size;
  new_chunk->free = false;
  new_chunk->padding = padding;
  new_chunk->magic = 0x7777;
  return new_chunk;
}

static void append_chunk(heapchunk_meta *append_to, heapchunk_meta *chunk) {
  if (append_to == chunk) {
    fprintf(stderr,
            "WARN: Attempted to append a chunk to itself, which would cause an "
            "infinite loop when searching. Aborting append_chunk(%p, %p).\n",
            (void *)append_to, (void *)chunk);
    return;
  }
  if (append_to != NULL) {
    append_to->next = chunk;
    return;
  }
  heapchunk_meta *curr_chunk = global_heap_base;
  while (curr_chunk->next != NULL) {
    curr_chunk = curr_chunk->next;
  }
  curr_chunk->next = chunk;
}

void *heapmalloc(size_t size) {
  if (size == 0)
    return NULL;

  heapchunk_meta *chunk;
  if (global_heap_base != NULL) {
    heapchunk_meta *last_chunk = global_heap_base;
    chunk = find_free_chunk(&last_chunk, size);
    if (chunk == NULL) {
      chunk = create_chunk(size);
      if (chunk == NULL)
        return NULL;
      append_chunk(last_chunk, chunk);
    }
    // TODO: Implement splitting blocks here.
    chunk->free = false;
  } else {
    chunk = create_chunk(size);
    if (chunk == NULL)
      return NULL;
    chunk->magic = 0xBA5E;
    global_heap_base = chunk;
  }
  return chunk + 1;
}

static heapchunk_meta *get_chunk_ptr(void *ptr) {
  return (heapchunk_meta *)ptr - 1;
}

// TODO: Implement merging continuous chunks.
void heapfree(void *ptr) {
  // Valid to call free with a NULL pointer.
  if (ptr == NULL)
    return;

  heapchunk_meta *chunk = get_chunk_ptr(ptr);
  assert(chunk->free == false);
  chunk->free = true;
  memset(ptr, FREE_BYTE, chunk->size);
}

void *heaprealloc(void *ptr, size_t size) {
  if (ptr == NULL)
    return heapmalloc(size);
  heapchunk_meta *old_chunk_meta = get_chunk_ptr(ptr);
  if (size <= old_chunk_meta->size) {
    // TODO: Implement resizing for smaller sizes.
    return ptr;
  }
  void *new_chunk = heapmalloc(size);
  if (new_chunk == NULL)
    return NULL; // TODO: Set errno on failure.
  memcpy(new_chunk, ptr, old_chunk_meta->size);
  heapfree(ptr);
  return new_chunk;
}

void *heapcalloc(size_t num_elements, size_t size_elements) {
  size_t size = num_elements * size_elements; // TODO: Check for overflow.
  void *new_chunk = heapmalloc(size);
  if (new_chunk == NULL)
    return NULL; // TODO: Set errno on failure.
  memset(new_chunk, 0, size);
  return new_chunk;
}
