#ifndef _HIGHLIGHTER_H
#define _HIGHLIGHTER_H

#include <string.h>
#include <pcre.h>

#include "ext/uthash/uthash.h"

#include "control.h"

#define MAX_HIGHLIGHTED_SUBSTRS 256

typedef struct highlighter_s {
    struct syntax_s* syntax;
    int* regex_ovector;
    struct highlighted_substr_s* highlighted_substrs;
} highlighter_t;

typedef struct highlighted_substr_s {
    int attrs;
    char* substr;
    int start_offset;
    int end_offset;
    int length;
} highlighted_substr_t;

typedef struct syntax_s {
    char* name;
    pcre* regex_file_pattern;
    struct syntax_rule_single_s* rule_single_head;
    struct syntax_rule_multi_s* rule_multi_head;
    UT_hash_handle hh;
} syntax_t;

typedef struct syntax_rule_single_s {
    pcre* regex;
    char* regex_str;
    int attrs;
    struct syntax_rule_single_s* next;
} syntax_rule_single_t;

typedef struct syntax_rule_multi_s {
    pcre* regex_start;
    pcre* regex_end;
    int attrs;
    struct syntax_rule_multi_s* next;
} syntax_rule_multi_t;

highlighter_t* highlighter_new(buffer_t* buffer, syntax_t* syntax);
highlighted_substr_t* highlighter_highlight(highlighter_t* self, char* line, int line_in_buffer, int* highlighted_substr_count);
void highlighter_on_dirty_lines(buffer_t* buffer, void* listener, int line_start, int line_end, int line_delta);

syntax_rule_single_t* syntax_rule_single_new(char* regex, int attrs);
syntax_rule_multi_t* syntax_rule_multi_new(char* regex_start, char* regex_end, int attrs);

extern syntax_t* syntaxes;

#endif
