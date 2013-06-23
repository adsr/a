#include "atto.h"

/**
 * Reverse version of memmem
 *
 * https://github.com/benolee/zile/blob/master/src/memrmem.c
 */
const char* memrmem(const char* s, size_t slen, const char* t, size_t tlen) {
    size_t i;
    if (slen >= tlen) {
        i = slen - tlen;
        do {
            if (memcmp(s + i, t, tlen) == 0) {
                return s + i;
            }
        } while (i-- != 0);
    }
    return NULL;
}
