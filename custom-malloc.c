#define _GNU_SOURCE // We need this for sbrk() to be declared.
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define META_SIZE sizeof(struct heapchunk_meta)

struct heapchunk_meta {
  struct heapchunk_meta *next;
  size_t size;
  unsigned short free;
};
typedef struct heapchunk_meta heapchunk_meta;

heapchunk_meta *global_heap_base = NULL;

heapchunk_meta *find_free_chunk(heapchunk_meta **last_chunk, size_t size) {
  heapchunk_meta *curr_chunk = global_heap_base;
  while (curr_chunk != NULL &&
         !(curr_chunk->free == 1 && curr_chunk->size >= size)) {
    *last_chunk = curr_chunk;
    curr_chunk = curr_chunk->next;
  };
  return curr_chunk;
}

heapchunk_meta *create_chunk(size_t size) {
  heapchunk_meta *new_chunk = sbrk(0);
  void *request = sbrk(META_SIZE + size);
  if (new_chunk == (void *)-1 || new_chunk != request)
    return NULL;
  new_chunk->size = size;
  new_chunk->free = 0;
  new_chunk->next = NULL;
  return new_chunk;
}

void append_chunk(heapchunk_meta *append_to, heapchunk_meta *chunk) {
  if (append_to == chunk) {
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
    // Can split blocks here.
    chunk->free = 0;
  } else {
    chunk = create_chunk(size);
    if (chunk == NULL)
      return NULL;
    global_heap_base = chunk;
  }
  return chunk + 1;
}

heapchunk_meta *get_chunk_ptr(void *ptr) { return (heapchunk_meta *)ptr - 1; }

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
  unsigned char garbage = 0xFE;
  memset(ptr, garbage, chunk->size);
}

void *realloc(void *ptr, size_t size) {
  if (ptr == NULL)
    return malloc(size);
  heapchunk_meta *old_chunk_meta = get_chunk_ptr(ptr);
  if (size <= old_chunk_meta->size) {
    // TODO: Implement resizing for smaller sizes.
    return ptr;
  }
  void *new_chunk = malloc(size);
  if (new_chunk == NULL)
    return NULL; // TODO: Set errno on failure.
  memcpy(new_chunk, ptr, old_chunk_meta->size);
  free(ptr);
  return new_chunk;
}

void *calloc(size_t num_elements, size_t size_elements) {
  size_t size = num_elements * size_elements; // TODO: Check for overflow.
  void *new_chunk = malloc(size);
  if (new_chunk == NULL)
    return NULL; // TODO: Set errno on failure.
  memset(new_chunk, 0, size);
  return new_chunk;
}
