# Malloc-in-C

A small reimplementation of the memory allocation functions available in C
(e.g. `malloc`, `realloc`, `calloc`, and `free`).

See the [commits](https://github.com/CupapiOT/malloc-in-c/commits/main/) to see
how each feature was integrated with the previous ones.

## Stack

- Currently uses C23, though C11 is sufficient as both versions contain
  `max_align_t`, needed for memory alignment. Initially used C99, and C23 was
  chosen arbitrarily. If you'd like to stick to C99, see
  [this post](https://www.reddit.com/r/C_Programming/comments/1dqwdqe/is_there_a_way_to_determine_sizeofmax_align_t_in/).
- GDB as the main debugging tool.
- AddressSanitizer for detecting memory errors.

## Sources

**DISCLAIMER**: I'm no expert, there may be incorrect information in these
sources, but they've helped me build this allocator. Make sure to do your own
research or fact-check these with an expert if you can!

These are all the references I found in the process of creating this custom
allocator so far (subject to change), organized into categories and referenced
like so: `1.2` means "1. General Tutorials: 2nd source." `3.x` refers to "3.
Endianness" as a whole. These aren't ordered in terms of importance; instead in
order of when I needed them to understand how to do the next step in expanding
this allocator's features. I hope that these can help you as well!

### 1. General Tutorials

These tutorials contain the general steps and overview in how to make your own
`malloc`. They won't explain _everything_, but they'll give you a sense of
what's important in an allocator. As for `1.1`, I at first couldn't replicate
the error found on first step of
[debugging](https://danluu.com/malloc-tutorial/#debugging), but I managed to
replicate it on my machine with a few workaround. I explain how
[here](./blogs/gdb-null-frees.c).

1. [Danluu: Malloc Tutorial](https://danluu.com/malloc-tutorial/)
2. [Low Level (YouTube): i wrote my own memory allocator in C to prove a point](https://youtu.be/CulF4YQt6zA)

### 2. LD Preload

These explain what `LD_PRELOAD` is as introduced in source `1.1`, used to
override system implementations of the memory allocation functions.

1. [Goldsborough: The LD_PRELOAD trick](https://www.goldsborough.me/c/low-level/kernel/2016/08/29/16-48-53-the_-ld_preload-_trick/)
2. [Linux Manual Pages: ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html)

### 3. Endianness

I had a few misconceptions as to what endianness was, which were:

- Endianness affects the ordering of _bits_ in memory. Endianness actually
  affects the ordering of _bytes_ in multi-byte values.
- It affects whether memory is laid out from lower-to-higher or higher-to-lower
  addresses in memory. This is an unrelated architectural decision.
- Hexadecimal values are somehow "special" with respect to endianness. In
  reality, hexadecimal is only a representation of a number; endianness only
  affects how that number is stored in memory.
- That big and little endian had no practical difference. They do.

These sources clarified those misunderstandings for me, and it matters for how
I implemented guard canaries (more on those in `5.x`).

1. [Wikipedia: Endianness](https://en.wikipedia.org/wiki/Endianness)
2. [@boubkerelmaayouf (Medium): Big Endian vs Little Endian — The Story, The Why, and Easy Explanation](https://medium.com/@boubkerelmaayouf/big-endian-vs-little-endian-the-story-the-why-and-easy-explanation-0c8ce4584edd)
3. [YC Hacker News: Big-endian vs. Little-endian](https://news.ycombinator.com/item?id=12471698)

### 4. Memory Alignment

I didn't understand what memory alignment really _meant_ before I read into
these. Basically, "CPUs always read memory at its word size" (`4.4`), and I
recommend reading `4.5` to understand what that means. `4.1` is more about
memory alignment in structs, but it helps to connect this concept to structs
because memory alignment doesn't only apply to memory allocation.

1. [Catb: The Lost Art of Structure Packing](http://www.catb.org/esr/structure-packing/)
2. [StackOverflow: Memory Alignment in C/C++](https://stackoverflow.com/questions/41719845/memory-alignment-in-c-c)
3. [StackOverflow: Purpose of memory alignment](https://stackoverflow.com/questions/381244/purpose-of-memory-alignment)
4. [SWE StackExchange: How important is memory alignment? Does it still matter?](https://softwareengineering.stackexchange.com/questions/328775/how-important-is-memory-alignment-does-it-still-matter)
5. [Wikipedia: Word (computer architecture)](<https://en.wikipedia.org/wiki/Word_(computer_architecture)>)
6. [Wikipedia: Data Structure Alignment](https://en.wikipedia.org/wiki/Data_structure_alignment)
7. [Reddit: Calling new or malloc, what is the real size reserved by the operating system?](https://www.reddit.com/r/cpp/comments/7n1dnt/calling_new_or_malloc_what_is_the_real_size/)
8. [Reddit: Is there a way to determine sizeof(max_align_t) in C99?](https://www.reddit.com/r/C_Programming/comments/1dqwdqe/is_there_a_way_to_determine_sizeofmax_align_t_in/)

### 5. Guard Canaries

These resources are actually for the stack and not the heap. As far as I know,
buffer canaries aren't implemented exactly as described in these sources. This
is because many production memory allocation functions (for optimization and
other reasons) rarely allocate exactly what you specify, often allocating more,
and so writing past what you allocated is undefined behavior. I implemented
guard canaries in one version of this allocator anyway out of curiosity.

Endianness (`3.x`) mattered here specifically because on little-endian
architectures like x86, `{0xCA, 0xFE, 0xBA, 0xBE}` gets read as `CA FE BA BE`
in memory while `0xCAFEBABE` is written as `BE BA FE CA`. `CA FE BA BE` is
clearly easier to recognize and debug.

1. [ctf101: Stack Canaries](https://ctf101.org/binary-exploitation/stack-canaries/)
2. [Wikipedia: Buffer overflow protection](https://en.wikipedia.org/wiki/Buffer_overflow_protection)

### Other Resources

These sources might not directly help you build your own `malloc`, but they
might help you understand memory allocation better in general.

#### Good Practices for `malloc`

1. [makecleanandmake: How to malloc() the Right Way](https://makecleanandmake.com/2014/07/26/how-to-malloc-the-right-way/)
2. [Reddit: What situations cause malloc to return NULL?](https://www.reddit.com/r/C_Programming/comments/iehy4l/what_situations_cause_malloc_to_return_null/)
3. [pvs-studio: Four reasons to check what the malloc function returned](https://pvs-studio.com/en/blog/posts/cpp/0938/)
4. [@Code_Analysis (Medium): Why it is bad idea to check result of malloc call with assert](https://medium.com/@Code_Analysis/why-it-is-bad-idea-to-check-result-of-malloc-call-with-assert-6b17e104ec0a)
5. [StackOverflow: Is it required to free a pointer variable before using realloc?](https://stackoverflow.com/questions/11548857/is-it-required-to-free-a-pointer-variable-before-using-realloc)

#### Arena Allocators

1. [dgtlgrove: Untangling Lifetimes](https://www.dgtlgrove.com/p/untangling-lifetimes-the-arena-allocator)
2. [nullprogram: Arena allocator tips and tricks](https://nullprogram.com/blog/2023/09/27/)

## License

As all the sources I found are indeed free online, and thus I'd like to keep
the code here free as well for anyone else to learn from. See
[LICENSE](LICENSE) for details.
