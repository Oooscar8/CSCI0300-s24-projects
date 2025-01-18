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
    off_t cache_start;   // File offset where cache starts
    size_t valid_bytes;  // Number of valid bytes in cache
    off_t current_pos;   // Current file position
    off_t file_offset;    // Actual file offset
    bool cache_dirty;  // True if cache has been modified and needs writing to disk.

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
                     "{desc:%s, fd:%d, cache_start:%ld, valid_bytes:%zu, current_pos:%ld, cache_dirty:%d, stats(r/w/s):%d/%d/%d} -- ", 
                     f->description,
                     f->fd,
                     f->cache_start,
                     f->valid_bytes,
                     f->current_pos,
                     f->cache_dirty,
                     f->stats.read_calls,
                     f->stats.write_calls,
                     f->stats.seeks);
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
    ret->file_offset = 0;  // File offset starts at 0
    ret->cache_start = 0;  // Cache starts at file beginning
    ret->valid_bytes = 0;  // No valid data in cache yet
    ret->cache_dirty = false;  // Cache starts clean

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

    // TODO: Implement this

    // Validate seek position
    if (pos < 0) return -1;

    // If seeking within current valid cache range, just update position
    if (pos >= f->cache_start && pos < f->cache_start + (off_t)f->valid_bytes) {
        f->current_pos = pos;
        return pos;
    }

    // If we have modified data in cache, flush it before moving
    if (f->cache_dirty) {
        if (io300_flush(f) == -1) return -1;
    }

    // Update our position tracking
    f->current_pos = pos;

    /* 
     * Invalidate cache that will trigger a fetch on next read
     * On next write, we can just start writing at the start of the cache
     * since we have already invalidated the cache and flushed any dirty data
     */
    f->cache_start = f->current_pos;
    f->valid_bytes = 0;

    // Return new position
    return pos;
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
    if (f->current_pos >= f->cache_start + (off_t)f->valid_bytes) {
        if (io300_fetch(f) < 0 || f->valid_bytes == 0) return -1;
    }

    return (unsigned char)f->cache[f->current_pos++ - f->cache_start];
}

int io300_writec(struct io300_file* f, int ch) {
    check_invariants(f);
    // TODO: Implement this

    if (f->current_pos >= f->cache_start + CACHE_SIZE) {
        if (f->cache_dirty) {
            if (io300_flush(f) == -1) return -1;
        }
        f->cache_start = f->current_pos;
        f->valid_bytes = 0;
    }

    f->cache[f->current_pos - f->cache_start] = ch;
    f->cache_dirty = true;
    f->current_pos++;
    if ((off_t)f->valid_bytes < f->current_pos - f->cache_start) {
        f->valid_bytes = f->current_pos - f->cache_start;
    }

    return ch;
}

ssize_t io300_read(struct io300_file* const f, char* const buff,
                   size_t const sz) {
    check_invariants(f);
    // TODO: Implement this

    if (f->current_pos >= io300_filesize(f)) return 0;

    /*
     * If the size of the data to be read is greater than the cache size, 
     * we can directly read it to the user buffer and skip the cache.
     */
    if (sz >= CACHE_SIZE) {
        if (f->cache_dirty && io300_flush(f) == -1) {
            return -1;
        }

        if (f->current_pos != f->file_offset) {
            f->file_offset = lseek(f->fd, f->current_pos, SEEK_SET);
            f->stats.seeks++;
        }
        
        ssize_t bytes = read(f->fd, buff, sz);
        f->stats.read_calls++;

        f->current_pos += bytes;
        f->file_offset += bytes;
        f->cache_start = f->current_pos;
        f->valid_bytes = 0;

        return bytes;
    }

    ssize_t total_read = 0;

    while (total_read < (ssize_t)sz) {
        // Calculate offset in cache for current position
        off_t pos_in_cache = f->current_pos - f->cache_start;

        // If current position is outside cache, fetch data from disk and fill cache first
        if (pos_in_cache >= (off_t)f->valid_bytes) {
            if (io300_fetch(f) < 0) return -1;
            if (f->valid_bytes == 0) return total_read;
            pos_in_cache = 0;
        }

        // Copy data to user buffer from cache
        size_t available = f->valid_bytes - pos_in_cache;
        size_t to_copy =
            sz - total_read < available ? sz - total_read : available;
        memcpy(buff + total_read, f->cache + pos_in_cache, to_copy);
        f->current_pos += to_copy;
        total_read += to_copy;
    }

    return total_read;
}
ssize_t io300_write(struct io300_file* const f, const char* buff,
                    size_t const sz) {
    check_invariants(f);
    // TODO: Implement this

    /*
     * If the size of the data to be written is greater than the cache size, 
     * we can directly write it to disk and skip the cache.
     */
    if (sz >= CACHE_SIZE) {
        if (f->cache_dirty && io300_flush(f) == -1) {
            return -1;
        }

        if (f->current_pos != f->file_offset) {
            f->file_offset = lseek(f->fd, f->current_pos, SEEK_SET);
            f->stats.seeks++;
        }

        ssize_t bytes = write(f->fd, buff, sz);
        f->stats.write_calls++;

        f->current_pos += bytes;
        f->file_offset += bytes;
        f->cache_start = f->current_pos;
        f->valid_bytes = 0;

        return bytes;
    }

    size_t total_written = 0;

    while (total_written < sz) {
        off_t pos_in_cache = f->current_pos - f->cache_start;

        /* 
         * If current position is outside cache, 
         * flush first and start writing at the beginning of the cache
         */
        if (pos_in_cache >= CACHE_SIZE) {
            if (f->cache_dirty && io300_flush(f) == -1) {
                return -1;
            }
            f->cache_start = f->current_pos;
            f->valid_bytes = 0;

            pos_in_cache = 0;
        }

        // Copy data to cache
        size_t available = CACHE_SIZE - pos_in_cache;
        size_t to_copy =
            sz - total_written < available ? sz - total_written : available;
        memcpy(f->cache + pos_in_cache, buff + total_written, to_copy);
        f->cache_dirty = true;
        f->current_pos += to_copy;
        if (f->valid_bytes < (size_t)pos_in_cache + to_copy) {
            f->valid_bytes = pos_in_cache + to_copy;
        }
        total_written += to_copy;
    }

    return total_written;
}

int io300_flush(struct io300_file* const f) {
    check_invariants(f);
    // TODO: Implement this

    if (!f->cache_dirty) return 0;

    // Seek to cache start and write valid bytes
    if (f->cache_start != f->file_offset) {
        f->file_offset = lseek(f->fd, f->cache_start, SEEK_SET);
        f->stats.seeks++;
    }
    
    ssize_t bytes = write(f->fd, f->cache, f->valid_bytes);
    f->stats.write_calls++;

    if (bytes < 0) return -1;  // Errors
    f->cache_dirty = false;
    f->file_offset += bytes;

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
    if (f->current_pos != f->file_offset) {
        f->file_offset = lseek(f->fd, f->current_pos, SEEK_SET);
        f->stats.seeks++;
    }
    f->cache_start = f->current_pos;

    ssize_t bytes = read(f->fd, f->cache, CACHE_SIZE);
    f->stats.read_calls++;

    if (bytes < 0) return -1;  // Errors
    f->valid_bytes = bytes;
    f->file_offset += bytes;

    return 0;
}
