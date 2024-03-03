#include "io300_test.h"

/*
 * Tests io300 file I/O implementations by creating a randomly generated file
 * (`io300_input`) of a given size, performing a given number of random file I/O
 * operations on it, of the given options, and verifying that those operations 
 * were performed correctly.
 * 
 * Takes in the following command line arguments:
 *  - A list of functions to test - either "all" (which includes `io300_seek`), or 
 *    any combination of "readc", "writec", "read", "write", or "seek". For "read" or "write", 
 *    by default a randomly sized buffer of size 1 to `MAX_BLOCK_SIZE` will be used for 
 *    each iteration. To specify a fixed size, one can format the argument instead as 
 *    "read=<size>" or "write=<size>".
 *  - Optionally, any of the following:
 *     - "-s <size>" to specify the size in bytes of the generated file (defaults to 8192).
 *     - "-m <size>" to specify the maximum size of the file - no writes or seeks will be
 *       performed beyond this boundary (defaults to 16384).
 *     - "-n <ops>" to specify the number of operations performed, not counting seeks (defaults 
 *       to 1000).
 *  
 */
int main(int argc, char* argv[]) {
    if (argc == 1) {
        usage(argv);
        exit(1);
    }

    // Process command arguments
    int opt;
    while ((opt = getopt(argc, argv, "s:m:n:")) != -1) {
        switch (opt) {
            case 's':
                if (interpret_size(optarg, &file_size, "file size", true) < 0)
                    exit(1);
                break;
            case 'm':
                if (interpret_size(optarg, &max_size, "maximum file size",
                                   false) < 0)
                    exit(1);
                break;
            case 'n':
                if (interpret_size(optarg, &num_ops, "number of operations",
                                   false) < 0)
                    exit(1);
                break;
            default: /* '?' */
                usage(argv);
                exit(1);
        }
    }

    if (max_size < file_size) {
        fprintf(stderr,
                "Error: maximum file size should be at least the starting file "
                "size\n");
        exit(1);
    }

    if (optind == argc) {
        fprintf(stderr, "Error: need to specify operations to test\n");
        exit(1);
    }

    for (int i = optind; i < argc; i++) {
        if (!strcmp(argv[i], "all")) {
            if (i == optind && i == argc - 1) {
                operations[0] = perform_readc;
                operations[1] = perform_writec;
                operations[2] = perform_read;
                operations[3] = perform_write;
                num_test_ops = 4;
                seeking = 1;
            } else {
                fprintf(
                    stderr,
                    "Error: \"all\" must be the only non-optional argument.\n");
                usage(argv);
                exit(1);
            }
        } else if (!strcmp(argv[i], "seek")) {
            seeking = 1;
        } else if (!strcmp(argv[i], "readc")) {
            operations[num_test_ops] = perform_readc;
            num_test_ops++;
        } else if (!strcmp(argv[i], "writec")) {
            operations[num_test_ops] = perform_writec;
            num_test_ops++;
        } else if (!strcmp(argv[i], "read")) {
            operations[num_test_ops] = perform_read;
            num_test_ops++;
        } else if (!strncmp(argv[i], "read=", 5)) {
            if (interpret_size(&argv[i][5], &read_size, "reading buffer size",
                               false) < 0)
                exit(1);
            if (read_size > MAX_BLOCK_SIZE) {
                fprintf(stderr,
                        "Error: specified buffer size for `io300_read` was "
                        "larger than maximum (%d)\n",
                        MAX_BLOCK_SIZE);
                exit(1);
            }
            operations[num_test_ops] = perform_read;
            num_test_ops++;
        } else if (!strcmp(argv[i], "write")) {
            operations[num_test_ops] = perform_write;
            num_test_ops++;
        } else if (!strncmp(argv[i], "write=", 6)) {
            if (interpret_size(&argv[i][6], &write_size, "writing buffer size",
                               false) < 0)
                exit(1);
            if (read_size > MAX_BLOCK_SIZE) {
                fprintf(stderr,
                        "Error: specified buffer size for `io300_write` was "
                        "larger than maximum (%d)\n",
                        MAX_BLOCK_SIZE);
                exit(1);
            }
            operations[num_test_ops] = perform_write;
            num_test_ops++;
        } else {
            fprintf(stderr, "Error: invalid command line argument \"%s\"\n",
                    argv[i]);
            exit(1);
        }
    }

    if (num_test_ops == 0) {
        fprintf(stderr, "Error: no specified file operations to test\n");
        usage(argv);
        exit(1);
    }

    // File contents generation
    unsigned char file_contents[max_size];
    if (generate_file(FILENAME, file_size, file_contents) < 0) exit(1);
    for (size_t i = file_size; i < max_size; i++) file_contents[i] = 0;

    // Perform operations
    struct io300_file* out = io300_open(FILENAME, "\e[0;32mout\e[0m");
    if (out == NULL) {
        fprintf(stderr, "io300_open: error opening file");
        cleanup();
        exit(1);
    }

    if (test_operations(out, file_contents) < 0) {
        cleanup();
        exit(1);
    }

    if (io300_close(out) != 0) {
        fprintf(stderr, "io300_close: error closing file");
        cleanup();
        exit(1);
    }

    // Testing correctness
    if (verify_contents(FILENAME, file_contents, file_size) < 0) {
        cleanup();
        exit(1);
    }

    cleanup();
    return 0;
}

/*
 * Prints a message detailing how to use this program, for the given list of command
 * line arguments `argv`.
 */
void usage(char* argv[]) {
    fprintf(stderr,
            "Usage: %s <commands> [-s <file-size>] [-m <max-file-size>] [-n "
            "<num-ops>]\n",
            argv[0]);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  all            : Test all functions\n");
    fprintf(stderr, "  readc          : Test `io300_readc`\n");
    fprintf(stderr, "  writec         : Test `io300_writec`\n");
    fprintf(stderr,
            "  read[=<size>]  : Test `io300_read`, with optionally a fixed "
            "given buffer size\n");
    fprintf(stderr,
            "  write[=<size>] : Test `io300_write`, with optionally a fixed "
            "given buffer size\n");
}

/*
 * For a given valid string, interprets the string as a `size_t` and stores
 * the value in `size_ptr`, with a description of the size's purpose/type contained
 * in the string `type`.
 * On success, returns 0; on failure, prints an appropriate error message and returns -1,
 * with the contents of `size_ptr` unchanged.
 */
int interpret_size(char* str, size_t* size_ptr, char* type, bool allow_zero) {
    char* endptr;
    long const size = strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Error: invalid %s \"%s\" provided.\n", type, str);
        return -1;
    }
    if (size < 0 || (size == 0 && !allow_zero)) {
        fprintf(stderr,
                "Error: invalid %s \"%s\" provided, must be positive value.\n",
                type, str);
        return -1;
    }

    *size_ptr = (size_t)size;
    return 0;
}

/*
 * Generates a file at the given file path composed of random bytes 
 * of the given size, and fills the given buffer with said bytes. 
 * (Assumes the `buffer` is at least `size` bytes long)
 * 
 * Returns 0 on success; on failure prints an appropriate error message
 * and returns -1.
 */
int generate_file(const char* path, size_t size, unsigned char* buffer) {
    FILE* out = fopen(path, "w");
    if (out == NULL) {
        perror("fopen");
        return -1;
    }

    srand(time(NULL));
    for (size_t i = 0; i < size; i++) {
        buffer[i] = rand() % UCHAR_MAX;
        if (fputc(buffer[i], out) == EOF) {
            perror("fputc");
            return -1;
        }
    }

    if (fclose(out) != 0) {
        perror("fclose");
        return -1;
    }

    return 0;
}

/*
 * Tests the operations as specified by the command line arguments - on 
 * the given file, performs the specified operations (stored in `operations`)
 * `num_ops` times, with buffer sizes given by `read_size` and `write_size`. 
 * Writes and seeks are not performed beyond `max_size` and if `seeking` is set,
 * then all operations are preceded by a call to `io300_seek`.
 * Any operations are also replicated on the contents of `file_contents`, with
 * size `max_size` and starts with the contents of the opened `file`.
 * 
 * Returns 0 if the verification succeeds; otherwise prints an appropriate 
 * error message and returns -1.
 */
int test_operations(struct io300_file* file, unsigned char* contents) {
    size_t loc = 0;
    for (size_t i = 0; i < num_ops; i++) {
        // seek if needed
        if (seeking) {
            loc = rand() % max_size;
            if (io300_seek(file, loc) < 0) {
                fprintf(stderr, "Error: `io300_seek` failed\n");
                return -1;
            }
        }

        // randomly choose an operation to test
        if (operations[rand() % num_test_ops](file, &loc, contents) < 0)
            return -1;
    }
    return 0;
}

/*
 * Performs/tests `io300_readc` using the given opened file, using/updating a pointer to 
 * the location of file read/write head `loc_ptr` and the current contents of the 
 * file.
 * 
 * Returns 0 if the test succeeds, and on failure prints an appropriate error message 
 * and returns -1.
 */
int perform_readc(struct io300_file* file, size_t* loc_ptr,
                  unsigned char* contents) {
    int byte_read = io300_readc(file);
    if (byte_read < 0) {
        if (*loc_ptr < file_size) {
            fprintf(
                stderr,
                "Error: `io300_readc` returned -1 when not at end of file\n");
            return -1;
        }
    } else {
        if (*loc_ptr == file_size) {
            fprintf(stderr,
                    "Error: `io300_readc` did not return -1 at end of file\n");
            return -1;
        }
        if (byte_read != contents[*loc_ptr]) {
            fprintf(stderr,
                    "Error: byte returned by `io300_readc` did not match that "
                    "at the current file location\n");
            return -1;
        }
        (*loc_ptr)++;
    }
    return 0;
}

/*
 * Performs/tests `io300_writec` using the given opened file, using/updating a pointer to 
 * the location of file read/write head `loc_ptr` and the current contents of the 
 * file.
 * 
 * Returns 0 if the test succeeds, and on failure prints an appropriate error message 
 * and returns -1.
 */
int perform_writec(struct io300_file* file, size_t* loc_ptr,
                   unsigned char* contents) {
    if (*loc_ptr < max_size) {
        contents[*loc_ptr] = rand() % UCHAR_MAX;
        if (io300_writec(file, contents[*loc_ptr]) == -1) {
            fprintf(stderr, "Error: `io300_writec` returned -1\n");
            return 1;
        }
        (*loc_ptr)++;
        file_size = MAX(file_size, *loc_ptr);
    }
    return 0;
}

/*
 * Performs/tests `io300_read` using the given opened file, using/updating a pointer to 
 * the location of file read/write head `loc_ptr` and the current contents of the 
 * file.
 * 
 * Returns 0 if the test succeeds, and on failure prints an appropriate error message 
 * and returns -1.
 */
int perform_read(struct io300_file* file, size_t* loc_ptr,
                 unsigned char* contents) {
    int buffer_size =
        read_size > 0 ? (int)read_size : (rand() % MAX_BLOCK_SIZE) + 1;
    unsigned char buffer[buffer_size];

    int bytes_read = io300_read(file, (char*)buffer, buffer_size);
    if (bytes_read > buffer_size) {
        fprintf(stderr,
                "Error: `io300_read` read more bytes into buffer than its "
                "length\n");
        return -1;
    } else if (bytes_read < 0) {
        fprintf(stderr, "Error: `io300_read` returned an error\n");
        return -1;
    } else if (bytes_read == 0) {
        if (*loc_ptr < file_size) {
            fprintf(stderr,
                    "Error: `io300_read` returned 0 bytes read, but not at end "
                    "of file\n");
            return -1;
        }
    } else {
        if (*loc_ptr == file_size) {
            fprintf(stderr,
                    "Error: `io300_read` did not return 0 bytes read at end of "
                    "file\n");
            return -1;
        }
        if (*loc_ptr + bytes_read > file_size) {
            fprintf(stderr,
                    "Error: `io300_read` returned more bytes read than in rest "
                    "of file\n");
            return -1;
        }

        for (int j = 0; j < bytes_read; j++) {
            if (buffer[j] != contents[*loc_ptr + j]) {
                fprintf(stderr,
                        "Error: bytes returned by `io300_read` did not match "
                        "those at the current file location\n");
                return -1;
            }
        }
        *loc_ptr += bytes_read;
    }
    return 0;
}

/*
 * Performs/tests `io300_write` using the given opened file, using/updating a pointer to 
 * the location of file read/write head `loc_ptr` and the current contents of the 
 * file.
 * 
 * Returns 0 if the test succeeds, and on failure prints an appropriate error message 
 * and returns -1.
 */
int perform_write(struct io300_file* file, size_t* loc_ptr,
                  unsigned char* contents) {
    int buffer_size =
        write_size > 0 ? (int)write_size : (rand() % MAX_BLOCK_SIZE) + 1;
    if (*loc_ptr + buffer_size > max_size) buffer_size = max_size - *loc_ptr;
    for (int j = 0; j < buffer_size; j++)
        contents[*loc_ptr + j] = rand() % UCHAR_MAX;

    int bytes_written =
        io300_write(file, (char*)&contents[*loc_ptr], buffer_size);
    if (bytes_written > buffer_size) {
        fprintf(stderr,
                "Error: `io300_write` wrote more bytes from buffer than its "
                "length\n");
        return -1;
    } else if (bytes_written < buffer_size) {
        fprintf(stderr,
                "Error: `io300_write` did not write all bytes from buffer\n");
        return -1;
    } else if (bytes_written < 0) {
        fprintf(stderr, "Error: `io300_write` returned an error\n");
        return -1;
    }
    *loc_ptr += bytes_written;
    file_size = MAX(file_size, *loc_ptr);
    return 0;
}

/*
 * Verifies that the file contents for the file located at `path`
 * the same as that contained in `contents`, whose size is given by `size`.
 * Used to verify implementations of `io300_writec` and `io300_write`.
 * 
 * Returns 0 if the verification succeeds; otherwise prints an appropriate 
 * error message and returns -1.
 */
int verify_contents(const char* path, unsigned char* contents, size_t size) {
    FILE* in = fopen(path, "r");
    if (in == NULL) {
        perror("fopen");
        return -1;
    }

    size_t loc = 0;
    while (1) {
        int byte_read = fgetc(in);
        if (byte_read == EOF) {
            if (loc != size) {
                fprintf(stderr,
                        "Error: final file shorter than expected, reached end "
                        "of file unexpectedly\n");
                return -1;
            }
            break;
        }
        if (loc == size) {
            fprintf(stderr,
                    "Error: final file longer than expected, did not reach end "
                    "of file at expected location\n");
            return -1;
        }
        if (byte_read != contents[loc]) {
            fprintf(stderr,
                    "Error: byte in final file did not match that expected at "
                    "the current file location\n");
            return -1;
        }

        loc++;
    }

    if (fclose(in) != 0) {
        perror("fclose");
        return -1;
    }

    return 0;
}

/* Cleans up the test by destroying the input file. */
void cleanup() {
    if (unlink(FILENAME) < 0) {
        perror("unlink");
        exit(1);
    }
}
