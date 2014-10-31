cutils
======

Utilities for programming in C

memcheck
========
A basic leak detector for when utilities like valgrind are not available or are
impractical.  This is a header file you can #include in any single-source-file
project that "replaces" malloc, calloc, realloc and free with versions that
track the file, function and line number where an alloc occurs, and reports
back what wasn't freed before program termination (if you call the reporting
function).
