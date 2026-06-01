#define _GNU_SOURCE // We need this for sbrk() to be declared.
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ALIGN_BYTES _Alignof(max_align_t) // C11 or above.
#define META_SIZE sizeof(struct heapchunk_meta)
#define FREE_BYTE 0xFE

struct heapchunk_meta {
  struct heapchunk_meta *next;
  size_t size;
  unsigned char free;
  unsigned char padding;
};
typedef struct heapchunk_meta heapchunk_meta;

static heapchunk_meta *global_heap_base = NULL;

static heapchunk_meta *find_free_chunk(heapchunk_meta **last_chunk,
                                       size_t size) {
  heapchunk_meta *curr_chunk = global_heap_base;
  while (curr_chunk != NULL &&
         !(curr_chunk->free == 1 && curr_chunk->size >= size)) {
    *last_chunk = curr_chunk;
    curr_chunk = curr_chunk->next;
  };
  return curr_chunk;
}

static heapchunk_meta *create_chunk(size_t size) {
  void *curr = sbrk(0);

  size_t ptr_addr = (size_t)curr + META_SIZE;
  size_t padding =
      (ALIGN_BYTES - (ptr_addr & (ALIGN_BYTES - 1))) & (ALIGN_BYTES - 1);
  size_t total_size = META_SIZE + padding + size;
  void *request = sbrk(total_size);

  if (curr == (void *)-1 || curr != request)
    return NULL;

  heapchunk_meta *new_chunk = (void *)((char *)curr + padding);

  new_chunk->next = NULL;
  new_chunk->size = size;
  new_chunk->free = 0;
  new_chunk->padding = padding;
  return new_chunk;
}

static void append_chunk(heapchunk_meta *append_to, heapchunk_meta *chunk) {
  if (append_to == chunk) {
    const char *append_to_self_msg =
        "append_chunk(): attempted to append a chunk to itself. This should "
        "never happen.\n";
    write(STDERR_FILENO, append_to_self_msg, strlen(append_to_self_msg));
    abort();
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

void *malloc(size_t size) {
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
    chunk->free = 0;
  } else {
    chunk = create_chunk(size);
    if (chunk == NULL)
      return NULL;
    global_heap_base = chunk;
  }
  return chunk + 1;
}

static heapchunk_meta *get_chunk_ptr(void *ptr) {
  return (heapchunk_meta *)ptr - 1;
}

void free(void *ptr) {
  // Valid to call free with a NULL pointer.
  if (ptr == NULL)
    return;

  heapchunk_meta *chunk = get_chunk_ptr(ptr);
  if (chunk->free != 0) {
    char *err_msg = "free(): double free detected.\n";
    write(STDERR_FILENO, err_msg, strlen(err_msg));
    abort(); // A double free aborts the program.
  }

  chunk->free = 1;
  memset(ptr, FREE_BYTE, chunk->size);
}

void *realloc(void *ptr, size_t size) {
  if (ptr == NULL)
    return malloc(size);
  heapchunk_meta *old_chunk_meta = get_chunk_ptr(ptr);
  if (size <= old_chunk_meta->size) {
    return ptr;
  }
  void *new_chunk = malloc(size);
  if (new_chunk == NULL)
    return NULL;
  memcpy(new_chunk, ptr, old_chunk_meta->size);
  free(ptr);
  return new_chunk;
}

void *calloc(size_t num_elements, size_t size_elements) {
  size_t size = num_elements * size_elements;
  void *new_chunk = malloc(size);
  if (new_chunk == NULL)
    return NULL;
  memset(new_chunk, 0, size);
  return new_chunk;
}
