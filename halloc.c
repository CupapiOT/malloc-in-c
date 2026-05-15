#define _GNU_SOURCE // We need this for sbrk() to be declared.
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct heapinfo {
  void *start;
  size_t size;
};

void *heapmalloc(size_t size) {
  if (size == 0)
    return NULL;

  struct heapinfo *start = sbrk(sizeof(struct heapinfo));
  if (start == (void *)-1) { // Failed to allocate heap info.
    return NULL;
  }

  void *original = sbrk(0);
  start->start = original;
  start->size = size;
  void *ptr = sbrk(size);
  if (ptr == (void *)-1) {
    return NULL;
  }
  assert(original == ptr); // Not thread safe
  return ptr;
}

void heapfree(void *ptr) {
  // printf("-------\nFreeing\n-------\n"); // Necessary so printf allocations don't mess with the debug info.
  size_t heapinfo_size = sizeof(struct heapinfo);
  struct heapinfo *heap_info = ptr - sizeof(struct heapinfo);
  void *end_of_allocd = ptr + heap_info->size;
  //
  // printf("Curr address   : %p\n", sbrk(0));
  // printf("Next address   : %p\n", end_of_allocd);
  // printf("Allocated addrs: %p\n", heap_info->start);
  // printf("Allocated bytes: %zu\n", heap_info->size);
  // printf("Size of heapinfo  : %zu\n", heapinfo_size);
  //
  sbrk(-(heap_info->size + heapinfo_size));
  // printf("Addr after hree: %p\n", sbrk(0));
}
