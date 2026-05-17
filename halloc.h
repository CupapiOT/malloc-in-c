#include <stddef.h>

void *heapmalloc(size_t size);
void heapfree(void *ptr);
void inspect_all_chunks();
