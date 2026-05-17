#include "halloc.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *allocate_string(char string[]) {
  size_t string_len = strlen(string) + 1;
  char *str = heapmalloc(string_len);

  if (str == NULL) {
    fprintf(stderr, "Failed to malloc!\n");
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

int main() {
  char msg1_str[] = "100";
  char *msg = allocate_string(msg1_str);

  char msg3_str[] = "3";
  char *msg3 = allocate_string(msg3_str);
  heapfree(msg);

  char msg2_str[] = "20";
  char *msg2 = allocate_string(msg2_str);
  heapfree(msg2);

  char msg4_str[] = "400";
  char *msg4 = allocate_string(msg4_str);
  inspect_all_chunks();
  heapfree(msg3);
  heapfree(msg4);

  return EXIT_SUCCESS;
}

/* int main(int argc, char *argv[]) {
  if (argc <= 1) return EXIT_SUCCESS;
  printf("Program start.\n");

  char *msg = allocate_string(argv[1]);
  printf("%s", msg);

  if (argc <= 2) return EXIT_SUCCESS;


  if (argc <= 3) return EXIT_SUCCESS;

  char *msg3 = allocate_string(argv[3]);
  heapfree(msg);
  char *msg2 = allocate_string(argv[2]);
  printf("%s", msg2);
  heapfree(msg2);
  printf("%s", msg3);
  heapfree(msg3);

  return EXIT_SUCCESS;
} */
