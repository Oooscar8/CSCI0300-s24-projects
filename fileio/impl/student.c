#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../io300.h"

/*
    student.c
    Fill in the following stencils
*/

/*
    When starting, you might want to change this for testing on small files.
*/
#ifndef CACHE_SIZE
#define CACHE_SIZE 8
#endif

#if (CACHE_SIZE < 4)
#error "internal cache size should not be below 4."
#error "if you changed this during testing, that is fine."
#error "when handing in, make sure it is reset to the provided value"
#error "if this is not done, the autograder will not run"
#endif

/*
   This macro enables/disables the dbg() function. Use it to silence your
   debugging info.
   Use the dbg() function instead of printf debugging if you don't want to
   hunt down 30 printfs when you want to hand in
*/
#define DEBUG_PRINT 1
#define DEBUG_STATISTICS 1

struct io300_file {
    /* read,write,seek all take a file descriptor as a parameter */
    int fd;
    /* this will serve as our cache */
    char* cache;

    // TODO: Your properties go here
    off_t cache_start;  // File offset where cache starts
    off_t current_pos;  // Current file position
    bool cache_dirty;  // True if cache has been modified and needs writing to disk.
    //int valid_bytes;  // Number of valid bytes in cache
    //bool cache_valid;     // True if cache contains valid data for current position range([cache_start, cache_start + valid_bytes))

    /* Used for debugging, keep track of which io300_file is which */
    char* description;
    /* To tell if we are getting the performance we are expecting */
    struct io300_statistics {
        int read_calls;
        int write_calls;
        int seeks;
    } stats;
};

int io300_fetch(struct io300_file* const f);

/*
    Assert the properties that you would like your file to have at all times.
    Call this function frequently (like at the beginning of each function) to
    catch logical errors early on in development.
*/
static void check_invariants(struct io300_file* f) {
    assert(f != NULL);
    assert(f->cache != NULL);
    assert(f->fd >= 0);

    // TODO: Add more invariants
}

/*
    Wrapper around printf that provides information about the
    given file. You can silence this function with the DEBUG_PRINT macro.
*/
static void dbg(struct io300_file* f, char* fmt, ...) {
    (void)f;
    (void)fmt;
#if (DEBUG_PRINT == 1)
    static char buff[300];
    size_t const size = sizeof(buff);
    int n = snprintf(buff, size,
                     // TODO: Add the fields you want to print when debugging
                     "{desc:%s, } -- ", f->description);
    int const bytes_left = size - n;
    va_list args;
    va_start(args, fmt);
    vsnprintf(&buff[n], bytes_left, fmt, args);
    va_end(args);
    printf("%s", buff);
#endif
}

struct io300_file* io300_open(const char* const path, char* description) {
    if (path == NULL) {
        fprintf(stderr, "error: null file path\n");
        return NULL;
    }

    int const fd = open(path, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        fprintf(stderr, "error: could not open file: `%s`: %s\n", path,
                strerror(errno));
        return NULL;
    }

    struct io300_file* const ret = malloc(sizeof(*ret));
    if (ret == NULL) {
        fprintf(stderr, "error: could not allocate io300_file\n");
        close(fd);
        return NULL;
    }

    ret->fd = fd;
    ret->cache = malloc(CACHE_SIZE);
    if (ret->cache == NULL) {
        fprintf(stderr, "error: could not allocate file cache\n");
        close(ret->fd);
        free(ret);
        return NULL;
    }
    ret->description = description;
    // TODO: Initialize your file
    // Initialize metadata
    ret->current_pos = 0;  // Start at beginning of file
    ret->cache_start = -1;  // Cache starts at -1 to trigger fetch on first read/write
    ret->cache_dirty = false;  // Cache starts clean
    //ret->valid_bytes = 0;  // No valid data in cache yet
    //ret->cache_valid = false;    // Cache starts invalid

    // Initialize statistics
    ret->stats.read_calls = 0;
    ret->stats.write_calls = 0;
    ret->stats.seeks = 0;

    check_invariants(ret);
    dbg(ret, "Just finished initializing file from path: %s\n", path);
    return ret;
}

int io300_seek(struct io300_file* const f, off_t const pos) {
    check_invariants(f);
    f->stats.seeks++;

    // TODO: Implement this
    return lseek(f->fd, pos, SEEK_SET);
}

int io300_close(struct io300_file* const f) {
    check_invariants(f);

    // Flush any remaining dirty data
    if (f->cache_dirty) {
        if (io300_flush(f) == -1) return -1;
    }

#if (DEBUG_STATISTICS == 1)
    printf("stats: {desc: %s, read_calls: %d, write_calls: %d, seeks: %d}\n",
           f->description, f->stats.read_calls, f->stats.write_calls,
           f->stats.seeks);
#endif
    // TODO: Implement this

    close(f->fd);
    free(f->cache);
    free(f);
    return 0;
}

off_t io300_filesize(struct io300_file* const f) {
    check_invariants(f);
    struct stat s;
    int const r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode)) {
        return s.st_size;
    } else {
        return -1;
    }
}

int io300_readc(struct io300_file* const f) {
    check_invariants(f);
    // TODO: Implement this

    if (f->current_pos >= io300_filesize(f)) return -1;

    // Check if current position is in cache range
    if (f->cache_start == -1 || f->current_pos < f->cache_start ||
        f->current_pos >= f->cache_start + CACHE_SIZE) {
        if (io300_fetch(f) == -1) return -1;
    }

    return (unsigned char)f->cache[f->current_pos++ - f->cache_start];
}

int io300_writec(struct io300_file* f, int ch) {
    check_invariants(f);
    // TODO: Implement this

    if (f->cache_start == -1 || f->current_pos < f->cache_start ||
        f->current_pos >= f->cache_start + CACHE_SIZE) {
        if (f->cache_dirty) {
            if (io300_flush(f) == -1) return -1;
        }
        f->cache_start = f->current_pos;
    }

    f->cache[f->current_pos - f->cache_start] = ch;
    f->cache_dirty = true;
    f->current_pos++;

    return ch;
}

ssize_t io300_read(struct io300_file* const f, char* const buff,
                   size_t const sz) {
    check_invariants(f);
    // TODO: Implement this
    return read(f->fd, buff, sz);
}
ssize_t io300_write(struct io300_file* const f, const char* buff,
                    size_t const sz) {
    check_invariants(f);
    // TODO: Implement this
    return write(f->fd, buff, sz);
}

int io300_flush(struct io300_file* const f) {
    check_invariants(f);
    // TODO: Implement this

    if (!f->cache_dirty) return 0;

    // Seek to cache start and write valid bytes
    lseek(f->fd, f->cache_start, SEEK_SET);
    f->stats.seeks++;

    // Calculate bytes to flush: min(CACHE_SIZE, remaining bytes)
    size_t bytes_to_flush = CACHE_SIZE;
    off_t file_size = io300_filesize(f);
    if (f->cache_start + CACHE_SIZE > file_size) {
        bytes_to_flush = file_size - f->cache_start;
    }

    if (write(f->fd, f->cache, bytes_to_flush) == -1) return -1;
    f->stats.write_calls++;
    f->cache_dirty = false;
    return 0;
}

int io300_fetch(struct io300_file* const f) {
    check_invariants(f);
    // TODO: Implement this
    /* This helper should contain the logic for fetching data from the file into the cache. */
    /* Think about how you can use this helper to refactor out some of the logic in your read, write, and seek functions! */
    /* Feel free to add arguments if needed. */

    // Flush if needed
    if (f->cache_dirty && io300_flush(f) == -1) return -1;

    // Read new block at current position
    lseek(f->fd, f->current_pos, SEEK_SET);
    f->stats.seeks++;
    f->cache_start = f->current_pos;
    if (read(f->fd, f->cache, CACHE_SIZE) <= 0) return -1;
    f->stats.read_calls++;

    return 0;
}
