#define _GNU_SOURCE // We need this for sbrk() to be declared.
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

void NOT_ACCESSIBLE__free_good(void *ptr) {
  // Valid to call free with a NULL pointer.
  if (ptr == NULL)
    return;

  heapchunk_meta *chunk = get_chunk_ptr(ptr);
  if (chunk->free != 0) {
    char *err_msg = "Attempted to free a freed pointer!";
    write(STDERR_FILENO, err_msg, strlen(err_msg));
    return; // Should actually be an error.
  }
  chunk->free = 1;
  unsigned char garbage = 0xFE;
  memset(ptr, garbage, chunk->size);
}

void NOT_ACCESSIBLE__free_bad(void *ptr) {
  // // Valid to call free with a NULL pointer.
  // if (ptr == NULL)
  //   return;

  heapchunk_meta *chunk = get_chunk_ptr(ptr);
  if (chunk->free != 0) {
    char *err_msg = "Attempted to free a freed pointer!";
    write(STDERR_FILENO, err_msg, strlen(err_msg));
    return; // Should actually be an error.
  }
  chunk->free = 1;
  unsigned char garbage = 0xFE;
  memset(ptr, garbage, chunk->size);
}

void *free_address = &NOT_ACCESSIBLE__free_good;
size_t NOT_ACCESSIBLE__null_pointer_count = 0;

void NOT_ACCESSIBLE__free_function(void (*free)(void *ptr), void *ptr) {
  free(ptr);
}

/*
INFO: PROBLEM STATEMENT
I was going through this tutorial: `https://danluu.com/malloc-tutorial/`
At [this point](https://danluu.com/malloc-tutorial/#debugging), it talked about
how to debug with `gdb` and how, after modifying `free()` to no longer return
safely when given a NULL pointer, you should get this error:
```
$ gdb /bin/ls
(gdb) set environment LD_PRELOAD=./malloc.so
(gdb) run
Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7bd7dbd in free (ptr=0x0) at malloc.c:113
113       assert(block_ptr->free == 0);
``
I did not get this error. I got this instead:
```
$ gdb /bin/ls
(gdb) set environment LD_PRELOAD=./custom-malloc.so
(gdb) run
Starting program: /usr/bin/ls
During startup program terminated with signal SIGSEGV, Segmentation fault.
```

INFO: GATHERING INFORMATION
After some digging around, I suspected that the program crashed before it could
even load any shared-object libraries. Conveniently, I then found out about
`set stop-on-solib-events 1` (and `show stop-on-solib-events`), which stops
program execution on Shared Object Library Events as if they were breakpoints.
Doing this on the correct version of this file and continuing after every stop
yields nothing special: `Inferior loaded ./custom-malloc.so` is shown, and `ls`
works as intended. Running this on a version with a bad `free`, however,
crashed the program immediately. So, `gdb` did indeed crash before any `.so`
files could be loaded.
*/
void free(void *ptr) {
  NOT_ACCESSIBLE__free_function(free_address, ptr);
  if (ptr == NULL) {
    // INFO: MY SOLUTION
    // I first tried setting `LD_PRELOAD` to a poisoned version halfway through
    // gdb, after the `Inferior loaded ...` message. This didn't work because
    // `LD_PRELOAD` is only read once. After a bit of thinking, I realized that
    // `free` doesn't actually need to really be `free`. It only needs the name
    // `free`. So I changed it around as seen in this file!
    // With the write() statement below (not printf because that itself uses
    // malloc), I simply counted the number of times it triggered, without
    // assigning `free_address` to `&NOT_ACCESSIBLE__free_bad` for now.
    // Counting the output from `run`:
    /* $ gdb /bin/ls
       (gdb) set environment LD_PRELOAD=./custom-malloc.so // A non-poisoned version of this file
       (gdb) set stop-on-solib-events 1
       (gdb) run
       Starting program: /usr/bin/ls
       Attempted to free a NULL pointer. // 1
       Attempted to free a NULL pointer. // 2
       Attempted to free a NULL pointer. // 3
       Attempted to free a NULL pointer. // 4
       Attempted to free a NULL pointer. // 5
       Attempted to free a NULL pointer. // 6
       Attempted to free a NULL pointer. // 7
       Attempted to free a NULL pointer. // 8
       Attempted to free a NULL pointer. // 9
       Attempted to free a NULL pointer. // 10
       Attempted to free a NULL pointer. // 11
       Stopped due to shared library event (no libraries added or removed)
       (gdb) c
       Continuing.
       [Thread debugging using libthread_db enabled]
       Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
       Stopped due to shared library event:
         Inferior loaded /home/cup/Projects/malloc-in-c/custom-malloc.so
           /lib/x86_64-linux-gnu/libselinux.so.1
           /lib/x86_64-linux-gnu/libc.so.6
           /lib/x86_64-linux-gnu/libpcre2-8.so.0
       (gdb) c
       Continuing.
       Attempted to free a NULL pointer. // 12
       Attempted to free a NULL pointer. // 13
       Attempted to free a NULL pointer. // 14
       Makefile  custom-malloc-bad.so  custom-malloc.c  custom-malloc.so  d.sh  halloc.c  halloc.h  other  r.sh  use_halloc.c
       Attempted to free a NULL pointer.
       [Inferior 1 (process 36300) exited normally]
    */
    char *null_pointer_msg = "Attempted to free a NULL pointer.\n";
    write(STDOUT_FILENO, null_pointer_msg, strlen(null_pointer_msg));

    // INFO:
    // So, `11` is exact number of times a NULL pointer is dereferenced before
    // custom-malloc.so is loaded (14 times total), at least for `/bin/ls`.
    if (++NOT_ACCESSIBLE__null_pointer_count == 11) {
      // To see all NULL pointer frees, comment this line.
      free_address = &NOT_ACCESSIBLE__free_bad;
    }

    // Running this file with that poisoning-assignment in effect, I get this:
    /*
    (gdb) set env LD_PRELOAD=/home/cup/Projects/malloc-in-c/custom-malloc.so
    (gdb) set stop-on-solib-events 1
    (gdb) r
    Starting program: /usr/bin/ls
    Attempted to free a NULL pointer. // repeated 11x
    Stopped due to shared library event (no libraries added or removed)
    (gdb) c
    Continuing.
    [Thread debugging using libthread_db enabled]
    Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
    Stopped due to shared library event:
      Inferior loaded /home/cup/Projects/malloc-in-c/custom-malloc.so
        /lib/x86_64-linux-gnu/libselinux.so.1
        /lib/x86_64-linux-gnu/libc.so.6
        /lib/x86_64-linux-gnu/libpcre2-8.so.0
    (gdb) c
    Continuing.
    Attempted to free a NULL pointer. // repeated 3x
    Makefile  custom-malloc-bad.so  custom-malloc.c  custom-malloc.so  d.sh  halloc.c  halloc.h  other  r.sh  use_halloc.c
    Attempted to free a NULL pointer.

    Program received signal SIGSEGV, Segmentation fault.
    0x00007ffff7fb952d in NOT_ACCESSIBLE__free_bad (ptr=0x0) at custom-malloc.c:101
    101       if (chunk->free != 0) {
    (gdb) list
    96        // // Valid to call free with a NULL pointer.
    97        // if (ptr == NULL)
    98        //   return;
    99
    100       heapchunk_meta *chunk = get_chunk_ptr(ptr);
    101       if (chunk->free != 0) {
    102         char *err_msg = "Attempted to free a freed pointer!";
    103         write(STDERR_FILENO, err_msg, strlen(err_msg));
    104         return; // Should actually be an error.
    105       }
    (gdb) p ptr
    $1 = (void *) 0x0
    (gdb) p chunk
    $2 = (heapchunk_meta *) 0xffffffffffffffe8
    */
    // And THAT matches the tutorial! Mission success.
    /*
    NOTE:
    I also tested this same trick with `/bin/echo`, which has 12 null
    dereferences before loading this file. Weirdly enough though, `echo`
    just didn't crash. I modified a part of the code above like so when testing
    `echo`:
    ```
    char *null_pointer_msg = "Attempted to free a NULL pointer.\n";
    write(STDOUT_FILENO, null_pointer_msg, strlen(null_pointer_msg));
    // The exact number of times a NULL pointer is freed before
    // `custom-malloc.so` is loaded for `/bin/echo`. If this is set to any
    // number less than 12, it segfaults.
    if (++NOT_ACCESSIBLE___null_pointer_count >= 12) {
      // Only prints once despite the total number of null-frees being
      // 14. Note that the above `null_pointer_msg` still prints every time.
      // Finding out *why* echo doesn't break and why it only prints once is
      // considered not worth the author's time (despite the author's
      // curiosity) and is left as an exercise to the reader.
      char *new_null_pointer_msg = "POISONED!\n";
      write(STDOUT_FILENO, new_null_pointer_msg, strlen(new_null_pointer_msg));
      free_address = &NOT_ACCESSIBLE__free_bad;
    }
    ```
    */
  }
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
