#ifndef _HIGHLIGHTER_H
#define _HIGHLIGHTER_H

#include <string.h>
#include <pcre.h>

#include "ext/uthash/uthash.h"

#include "control.h"
#include "buffer.h"

#define MAX_HIGHLIGHTED_SUBSTRS 256

typedef struct highlighter_s {
    struct syntax_s* syntax;
    int* regex_ovector;
    struct highlighted_substr_s* highlighted_substrs;
    buffer_t* buffer;
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
    char* regex_start_str;
    char* regex_end_str;
    int attrs;
    struct syntax_rule_multi_s* next;
    struct syntax_rule_multi_workspace_s* workspaces;
} syntax_rule_multi_t;

typedef struct syntax_rule_multi_workspace_s {
    buffer_t* buffer;
    void* buffer_key;
    UT_array* start_offsets;
    UT_array* end_offsets;
    UT_array* ranges;
    UT_hash_handle hh;
} syntax_rule_multi_workspace_t;

typedef struct syntax_rule_multi_range_s {
    bool active;
    bool whole_line;
    int start_offset;
    int end_offset;
    struct syntax_rule_multi_range_s* next;
} syntax_rule_multi_range_t;

highlighter_t* highlighter_new(buffer_t* buffer, syntax_t* syntax);
highlighted_substr_t* highlighter_highlight(highlighter_t* self, char* line, int line_in_buffer, int* highlighted_substr_count);
void highlighter_on_dirty_lines(buffer_t* buffer, void* listener, int line_start, int line_end, int line_delta);
int highlighter_update_offsets(buffer_t* buffer, pcre* regex, int look_offset, UT_array* offsets, bool is_end_offset);
int highlighter_update_lines(syntax_rule_multi_workspace_t* workspace, int line_start, int line_end);
syntax_rule_single_t* syntax_rule_single_new(char* regex, int attrs);
syntax_rule_multi_t* syntax_rule_multi_new(char* regex_start, char* regex_end, int attrs);
syntax_rule_multi_workspace_t* syntax_rule_multi_workspace_find_or_new(syntax_rule_multi_t* rule, buffer_t* buffer);
int syntax_rule_multi_add_range(syntax_rule_multi_workspace_t* workspace, int line, int start_offset, int end_offset);
int syntax_rule_multi_range_make(syntax_rule_multi_range_t** range_ref, int start_offset, int end_offset);
void syntax_rule_multi_range_dtor(void* el);
bool syntax_rule_multi_lines_contain_match(syntax_rule_multi_t* rule, buffer_t* buffer, int line_start, int line_end);

extern syntax_t* syntaxes;

#endif
