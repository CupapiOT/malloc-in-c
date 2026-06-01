#include <stddef.h>
#include <stdbool.h>

void *heapmalloc(size_t size);
void heapfree(void *ptr);
void *heaprealloc(void *ptr, size_t size);
void *heapcalloc(size_t num, size_t size);
void inspect_all_chunks();
void heaphexdump();
