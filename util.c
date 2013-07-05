#include "atto.h"

/**
 * Reverse version of memmem
 *
 * https://github.com/benolee/zile/blob/master/src/memrmem.c
 */
const char* util_memrmem(const char* s, size_t slen, const char* t, size_t tlen) {
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

/**
 * Given a regex string, return a pcre pointer
 * Return NULL on error
 */
pcre* util_compile_regex(char* regex) {
    pcre* re;
    const char* err;
    int erroffset;
    err = NULL;
    erroffset = 0;
    re = pcre_compile(regex, PCRE_NO_AUTO_CAPTURE, &err, &erroffset, NULL);
    if (!re) {
        return NULL;
    }
    return re;
}

/**
 * Returns a color_pair for a fg and bg
 * If it does not already exist, the color pair is created
 * Returns 0 if there are too many color pairs defined
 */
int util_get_ncurses_color_pair(int fg_num, int bg_num) {
    static short pair_count = 0;
    short pair_start = 2;
    short pair_end = pair_start + pair_count - 1;
    short pair;
    short tmp_fg_num;
    short tmp_bg_num;

    for (pair = pair_start; pair <= pair_end; pair++) {
        pair_content(pair, &tmp_fg_num, &tmp_bg_num);
        if (tmp_fg_num == fg_num && tmp_bg_num == bg_num) {
            return COLOR_PAIR(pair);
        }
    }

    pair = pair_start + pair_count;
    if (pair >= COLOR_PAIRS) {
        // Too many pairs defined
        return 0;
    }

    init_pair(pair, fg_num, bg_num);
    pair_count += 1;
    return COLOR_PAIR(pair);
}
