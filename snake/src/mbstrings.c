#include "mbstrings.h"

#include <stddef.h>

/* mbslen - multi-byte string length
 * - Description: returns the number of UTF-8 code points ("characters")
 * in a multibyte string. If the argument is NULL or an invalid UTF-8
 * string is passed, returns -1.
 *
 * - Arguments: A pointer to a character array (`bytes`), consisting of UTF-8
 * variable-length encoded multibyte code points.
 *
 * - Return: returns the actual number of UTF-8 code points in `src`. If an
 * invalid sequence of bytes is encountered, return -1.
 *
 * - Hints:
 * UTF-8 characters are encoded in 1 to 4 bytes. The number of leading 1s in the
 * highest order byte indicates the length (in bytes) of the character. For
 * example, a character with the encoding 1111.... is 4 bytes long, a character
 * with the encoding 1110.... is 3 bytes long, and a character with the encoding
 * 1100.... is 2 bytes long. Single-byte UTF-8 characters were designed to be
 * compatible with ASCII. As such, the first bit of a 1-byte UTF-8 character is
 * 0.......
 *
 * You will need bitwise operations for this part of the assignment!
 */
size_t mbslen(const char* bytes) {
    // TODO: implement!
    if (bytes == NULL) {
        return -1;
    }
    size_t len = 0;
    while (*bytes) {
        char current_bytes = *bytes;
        int ones = 0;
        while ((current_bytes & (~current_bytes + 0b10000000)) == 0b10000000) {
            current_bytes = current_bytes << 1;
            ones += 1;
        }
        if (ones == 0) {
            bytes += 1;
        } else if (ones == 1) {
            return -1;
        } else {
            bytes += ones;
        }
        len += 1;
    }
    return len;
}
