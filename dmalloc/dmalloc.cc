#define DMALLOC_DISABLE 1
#include "dmalloc.hh"

#include <cassert>
#include <cstring>

struct dmalloc_stats m_stats = {0, 0, 0, 0, 0, 0, 0xffffffffffffffff, 0};
struct metadata {
    unsigned long long size;
};

/**
 * dmalloc(sz,file,line)
 *      malloc() wrapper. Dynamically allocate the requested amount `sz` of
 * memory and return a pointer to it
 *
 * @arg size_t sz : the amount of memory requested
 * @arg const char *file : a string containing the filename from which dmalloc
 * was called
 * @arg long line : the line number from which dmalloc was called
 *
 * @return a pointer to the heap where the memory was reserved
 */
void* dmalloc(size_t sz, const char* file, long line) {
    (void)file, (void)line;  // avoid uninitialized variable warnings
    // Your code here.
    void* ptr = base_malloc(sizeof(metadata) + sz);
    if (ptr == NULL) {
        m_stats.nfail += 1;
        m_stats.fail_size += sz;
        return NULL;
    }
    m_stats.nactive += 1;
    m_stats.ntotal += 1;
    m_stats.active_size += sz;
    m_stats.total_size += sz;
    if ((uintptr_t)((metadata*)ptr + 1) < m_stats.heap_min) {
        m_stats.heap_min = (uintptr_t)((metadata*)ptr + 1);
    }
    if ((uintptr_t)((metadata*)ptr + 1) > m_stats.heap_max) {
        m_stats.heap_max = (uintptr_t)((metadata*)ptr + 1);
    }
    struct metadata ptr_metadata = {(unsigned long long)sz};
    *(metadata*)ptr = ptr_metadata;
    return (metadata*)ptr + 1;
}

/**
 * dfree(ptr, file, line)
 *      free() wrapper. Release the block of heap memory pointed to by `ptr`.
 * This should be a pointer that was previously allocated on the heap. If `ptr`
 * is a nullptr do nothing.
 *
 * @arg void *ptr : a pointer to the heap
 * @arg const char *file : a string containing the filename from which dfree was
 * called
 * @arg long line : the line number from which dfree was called
 */
void dfree(void* ptr, const char* file, long line) {
    (void)file, (void)line;  // avoid uninitialized variable warnings
    // Your code here.
    if (ptr == NULL) {
        return;
    }
    m_stats.nactive -= 1;
    m_stats.active_size -= ((metadata*)ptr - 1)->size;
    base_free(ptr);
    base_free((metadata*)ptr - 1);
}

/**
 * dcalloc(nmemb, sz, file, line)
 *      calloc() wrapper. Dynamically allocate enough memory to store an array
 * of `nmemb` number of elements with wach element being `sz` bytes. The memory
 * should be initialized to zero
 *
 * @arg size_t nmemb : the number of items that space is requested for
 * @arg size_t sz : the size in bytes of the items that space is requested for
 * @arg const char *file : a string containing the filename from which dcalloc
 * was called
 * @arg long line : the line number from which dcalloc was called
 *
 * @return a pointer to the heap where the memory was reserved
 */
void* dcalloc(size_t nmemb, size_t sz, const char* file, long line) {
    // Your code here (to fix test014).
    void* ptr = dmalloc(nmemb * sz, file, line);
    if (ptr) {
        memset(ptr, 0, nmemb * sz);
    }
    return ptr;
}

/**
 * get_statistics(stats)
 *      fill a dmalloc_stats pointer with the current memory statistics
 *
 * @arg dmalloc_stats *stats : a pointer to the the dmalloc_stats struct we want
 * to fill
 */
void get_statistics(dmalloc_stats* stats) {
    // Stub: set all statistics to enormous numbers
    memset(stats, 255, sizeof(dmalloc_stats));
    // Your code here.
    *stats = m_stats;
}

/**
 * print_statistics()
 *      print the current memory statistics to stdout
 */
void print_statistics() {
    dmalloc_stats stats;
    get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/**
 * print_leak_report()
 *      Print a report of all currently-active allocated blocks of dynamic
 *      memory.
 */
void print_leak_report() {
    // Your code here.
}
