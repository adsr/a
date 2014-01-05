#include "atto.h"

/**
 * Allocate a single-line style
 */
srule_t* srule_new_single(char* regex, int color, int bg_color, int other_attrs) {
    srule_t* srule;
    pcre* re;
    re = util_compile_regex(regex);
    if (!re) {
        return NULL;
    }
    srule = _srule_new(color, bg_color, other_attrs);
    srule->type = ATTO_SRULE_TYPE_SINGLE;
    srule->regex = strdup(regex);
    srule->cregex = re;
    return srule;
}

/**
 * Allocate a multi-line style
 */
srule_t* srule_new_multi(char* regex_start, char* regex_end, int color, int bg_color, int other_attrs) {
    srule_t* srule;
    pcre* re;
    pcre* re_end;
    re = util_compile_regex(regex_start);
    if (!re) {
        return NULL;
    }
    re_end = util_compile_regex(regex_end);
    if (!re_end) {
        return NULL;
    }
    srule = _srule_new(color, bg_color, other_attrs);
    srule->type = ATTO_SRULE_TYPE_MULTI;
    srule->regex = strdup(regex_start);
    srule->regex_end = strdup(regex_end);
    srule->cregex = re;
    srule->cregex_end = re_end;
    return srule;
}

/**
 * Allocate a range style
 */
srule_t* srule_new_range(mark_t* start, mark_t* end, int color, int bg_color, int other_attrs) {
    srule_t* srule;
    srule = _srule_new(color, bg_color, other_attrs);
    srule->type = ATTO_SRULE_TYPE_RANGE;
    srule->range_start = start;
    srule->range_end = end;
    return srule;
}

/**
 * Allocate a style
 */
srule_t* _srule_new(int color, int bg_color, int other_attrs) {
    srule_t* srule;
    srule = (srule_t*)calloc(1, sizeof(srule_t));
    // TODO ncurses color pairs
    srule->color = color;
    srule->bg_color = bg_color;
    srule->other_attrs = other_attrs;
    srule->attrs = util_get_ncurses_color_pair(color, bg_color) + other_attrs;
    return srule;
}

/**
 * Free a style
 */
int srule_destroy(srule_t* self) {
    if (self->cregex) {
        pcre_free(self->cregex);
    }
    if (self->cregex_end) {
        pcre_free(self->cregex_end);
    }
    if (self->regex) {
        free(self->regex);
    }
    if (self->regex_end) {
        free(self->regex_end);
    }
    free(self);
    return ATTO_RC_OK;
}
