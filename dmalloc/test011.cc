#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dmalloc.hh"
// heap_min and heap_max checking, no overlap with other regions.

static int global;

int main() {
    for (int i = 0; i != 100; ++i) {
        size_t sz = rand() % 100;
        char* p = (char*)malloc(sz);
        free(p);
    }
    dmalloc_stats stat;
    get_statistics(&stat);

    // We haven't covered unions in class. They're similar to structs, yet each
    // member shares the same underlying memory:
    // https://en.cppreference.com/w/cpp/language/union
    //
    // In other words, `addr`, `iptr`, `statptr`, and `mainptr` here are
    // *alternatives* (rather than different fields), and all will be stored in
    // the same memory.
    union {
        uintptr_t addr;
        int* iptr;
        dmalloc_stats* statptr;
        int (*mainptr)();  // This is a function pointer. It holds the address
                           // of the beginning of the main function (which is in
                           // the code segment)
    } x;
    x.iptr = &global;
    assert(x.addr + sizeof(int) < stat.heap_min || x.addr >= stat.heap_max);
    x.statptr = &stat;
    assert(x.addr + sizeof(int) < stat.heap_min || x.addr >= stat.heap_max);
    x.mainptr = &main;
    assert(x.addr + sizeof(int) < stat.heap_min || x.addr >= stat.heap_max);
}
