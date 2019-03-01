Readme for Heap allocation:

There are some test cases to test the file.

To build the code, change to your top level assignment directory and type:
make
Once you have the library, you can use it to override the existing malloc by using
LD_PRELOAD:
$ env LD_PRELOAD=lib/libmalloc-ff.so cat README.md
or
$ env LD_PRELOAD=lib/libmalloc-ff.so tests/test1

To run the other heap management schemes replace libmalloc-ff.so with the appropriate
library:
Best-Fit:
First-Fit:
Next-Fit:
Worst-Fit:
libmalloc-bf.so
libmalloc-ff.so
libmalloc-nf.so
libmalloc-wf.so
