#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../io300.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MAX_BLOCK_SIZE 30

typedef int (*perform_func)(struct io300_file* file, size_t* loc_ptr,
                            unsigned char* contents);

/* Name of the file tested on. */
const char* FILENAME = "/tmp/io300_input";

/* The starting size of the generated file. */
size_t file_size = 8192;

/* The maximum size of the generated file. */
size_t max_size = 16384;

/* The number of operations performed on the generated file. */
size_t num_ops = 1000;

/* Whether or not `io300_seek` should be performed. */
int seeking = 0;

/* The functions that perform/test the chosen operations to test. */
perform_func operations[4] = {NULL, NULL, NULL, NULL};

/* The number of operations to test (i.e. the number of non-`NULL` elements in `operations) */
int num_test_ops = 0;

/* 
   The size of the buffer used for `io300_read`, or 0 for a random size 
   for each iteration.
*/
size_t read_size = 0;

/* 
   The size of the buffer used for `io300_write`, or 0 for a random size 
   for each iteration.
*/
size_t write_size = 0;

void usage(char* argv[]);

int interpret_size(char* str, size_t* ptr, char* type, bool allow_zero);

int generate_file(const char* path, size_t size, unsigned char* buffer);
int test_operations(struct io300_file* file, unsigned char* file_contents);

int perform_readc(struct io300_file* file, size_t* loc_ptr,
                  unsigned char* contents);
int perform_writec(struct io300_file* file, size_t* loc_ptr,
                   unsigned char* contents);
int perform_read(struct io300_file* file, size_t* loc_ptr,
                 unsigned char* contents);
int perform_write(struct io300_file* file, size_t* loc_ptr,
                  unsigned char* contents);

int verify_contents(const char* path, unsigned char* contents, size_t size);

void cleanup();
