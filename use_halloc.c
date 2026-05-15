#include "halloc.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *allocate_argv(char *argv) {
  size_t argv_len = strlen(argv);
  char template[] = "Hello, %s. The size of this message is %zu bytes.\n";
  size_t template_len = strlen(template) - 3; // Subtract `d` and `zu`.
  size_t msg_len = argv_len + template_len + 1; // Make room for null character.
  char *msg = heapmalloc(msg_len);
  
  if (msg == NULL) {
    fprintf(stderr, "Failed to halloc!\n");
    return NULL;
  }

  snprintf(msg, msg_len, template, argv, msg_len);
  return msg;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) return EXIT_SUCCESS;
  printf("Program start.\n");

  char *msg = allocate_argv(argv[1]);
  printf("%s", msg);

  if (argc <= 2) return EXIT_SUCCESS;


  if (argc <= 3) return EXIT_SUCCESS;

  char *msg3 = allocate_argv(argv[3]);
  heapfree(msg);
  char *msg2 = allocate_argv(argv[2]);
  printf("%s", msg2);
  heapfree(msg2);
  printf("%s", msg3);
  heapfree(msg3);
  
  return EXIT_SUCCESS;
}
