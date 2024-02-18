#include <cassert>
#include <cstdio>
#include <cstring>

#include "dmalloc.hh"

// Nasty large callocs.

struct large_struct {
    short a[0x80000001UL];
};

int main() {
    size_t huge_number = 0x8000000000000001UL;
    size_t large_number = 0x100000001UL;

    // A huge amount of small objects
    void* p1 = calloc(huge_number, 2);
    assert(p1 == nullptr);

    // A small amount of huge objects
    void* p2 = calloc(2, huge_number);
    assert(p2 == nullptr);

    // A large amount of large objects
    void* p3 = calloc(large_number, sizeof(large_struct));
    assert(p3 == nullptr);

    print_statistics();
}

//! alloc count: active          0   total          0   fail          3
//! alloc size:  active          0   total          0   fail        ???