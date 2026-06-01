#include "halloc.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char *allocate_string(char string[]) {
  size_t string_len = strlen(string) + 1;
  char *str = heapmalloc(string_len);

  if (str == NULL) {
    fprintf(stderr, "Failed to heapmalloc!\n");
    return NULL;
  }

  snprintf(str, string_len, "%s", string);
  return str;
}

char *mallocate_string(char string[]) {
  size_t string_len = strlen(string) + 1;
  char *str = malloc(string_len);

  if (str == NULL) {
    fprintf(stderr, "Failed to malloc!\n");
    return NULL;
  }

  snprintf(str, string_len, "%s", string);
  return str;
}

size_t num_of_times_allocated = 0;
const size_t DATA = 0xD0;

void *allocate_bytes(size_t size) {
  void *h = heapmalloc(size);
  if (h == NULL) {
    fprintf(stderr, "Failed to heapmalloc!\n");
    abort();
  }
  const size_t ALIGN = 16;
  assert((size_t)h % ALIGN == 0);
  memset(h, DATA + num_of_times_allocated++, size);
  return h;
}

// Credit: https://stackoverflow.com/questions/2509679/
// Returns [1, max]
size_t random_at_most(size_t max) {
  size_t num_bins = (size_t)max + 1, num_rand = (size_t)RAND_MAX + 1,
         bin_size = num_rand / num_bins, defect = num_rand % num_bins;
  size_t x;
  do {
    x = rand();
  } while (num_rand - defect <= (size_t)x);
  return (x / bin_size) + 1;
}

int test_16byte_alignment() {
  srand(time(NULL));
  printf("TEST_16BYTE_ALIGNMENT START\n");
  size_t largest_chunk = 64;
  void *h1 = allocate_bytes(random_at_most(largest_chunk));
  void *h2 = allocate_bytes(random_at_most(largest_chunk));
  void *h3 = allocate_bytes(random_at_most(largest_chunk));
  void *h4 = allocate_bytes(random_at_most(largest_chunk));
  void *h5 = allocate_bytes(random_at_most(largest_chunk));
  void *h6 = allocate_bytes(random_at_most(largest_chunk));
  heapfree(h1);
  heapfree(h2);
  heapfree(h3);
  heapfree(h4);
  heapfree(h5);
  heapfree(h6);
  void *h7 = allocate_bytes(random_at_most(largest_chunk));
  void *h8 = allocate_bytes(random_at_most(largest_chunk));
  void *h9 = allocate_bytes(random_at_most(largest_chunk));
  void *h10 = allocate_bytes(random_at_most(largest_chunk));
  void *h11 = allocate_bytes(random_at_most(largest_chunk));
  void *h12 = allocate_bytes(random_at_most(largest_chunk));
  heapfree(h7);
  heapfree(h8);
  heapfree(h9);
  heapfree(h10);
  heapfree(h11);
  heapfree(h12);
  printf("TEST_16BYTE_ALIGNMENT SUCCESS\n");
  return 0;
}

int test_realloc_and_calloc() {
  printf("TEST_REALLOC_AND_CALLOC START\n");
  char msg1_str[] = "100";
  char *msg1 = allocate_string(msg1_str);

  size_t new_msg1_len = strlen(msg1_str) + 2; // Make room for 1 extra char.
  char *tmp1 = heaprealloc(msg1, new_msg1_len);
  if (tmp1 == NULL) {
    fprintf(stderr, "Failed to heaprealloc!");
    return 1;
  }
  msg1 = tmp1;
  char *msg1_m = mallocate_string(msg1_str);
  char *tmp_m = realloc(msg1_m, new_msg1_len);
  if (tmp_m == NULL) {
    fprintf(stderr, "Failed to realloc!");
    return 1;
  }
  msg1_m = tmp_m;
  assert(strcmp(msg1, msg1_m) == 0);

  char msg3_str[] = "3";
  char *msg3 = allocate_string(msg3_str);
  heapfree(msg1);

  char msg2_str[] = "2000";
  char *msg2 = allocate_string(msg2_str);
  heapfree(msg2);

  char msg4_str[] = "400";
  char *msg4 = allocate_string(msg4_str);
  heapfree(msg3);
  heapfree(msg4);
  int *nums = heapcalloc(10, sizeof *nums);
  nums[0] = 2;
  nums[1] = 3;
  nums[2] = 5;
  nums[3] = 7;
  heapfree(nums);
  // heaphexdump();
  // inspect_all_chunks();
  printf("TEST_REALLOC_AND_CALLOC SUCCESS\n");
  return EXIT_SUCCESS;
}

int test_basic_functionality(int argc, char *argv[]) {
  if (argc <= 1)
    return EXIT_SUCCESS;
  printf("Program start.\n");

  char *msg = allocate_string(argv[1]);
  printf("%s", msg);

  if (argc <= 2)
    return EXIT_SUCCESS;

  if (argc <= 3)
    return EXIT_SUCCESS;

  char *msg3 = allocate_string(argv[3]);
  heapfree(msg);
  char *msg2 = allocate_string(argv[2]);
  printf("%s", msg2);
  heapfree(msg2);
  printf("%s", msg3);
  heapfree(msg3);

  return EXIT_SUCCESS;
}

int main() {
  // test_basic_functionality(argc, argv);

  test_realloc_and_calloc();

  test_16byte_alignment();
}
