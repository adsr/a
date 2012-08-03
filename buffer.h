#ifndef _BUFFER_H
#define _BUFFER_H

#include "ext/bstrlib/bstrlib.h"
#include "ext/uthash/utarray.h"

#include "control.h"

typedef struct buffer_listener_s {
    struct control_s* control;
    struct buffer_listener_s* next;
} buffer_listener_t;

typedef struct buffer_s {
    bstring buffer;
    unsigned int char_count;
    unsigned int line_count;
    UT_array* line_offsets;
    buffer_listener_t* buffer_listener_head;
} buffer_t;


int buffer_insert_str(
    buffer_t* buffer,
    int line,
    int offset,
    const char* str,
    int chars_to_delete,
    int* new_line,
    int* new_offset
);

buffer_t* buffer_new();
int buffer_get_buffer_offset(buffer_t* buffer, int line, int offset);
int buffer_calc_line_offsets(buffer_t* buffer);
int buffer_dirty_lines(buffer_t* buffer, int line_start, int line_end, bool line_count_decreased);
int buffer_get_line_and_offset(buffer_t* buffer, int new_buffer_offset, int* new_line, int* new_offset);
int buffer_add_listener(buffer_t* buffer, struct control_s*);
int buffer_load_from_file(buffer_t* buffer, char* filename);

char** buffer_get_lines(buffer_t* buffer, int dirty_line_start, int dirty_line_end, int* line_count);
char* buffer_get_line(buffer_t* buffer, int line);
int buffer_get_line_offset_and_length(buffer_t* buffer, int line, int* start_offset, int* length);

#endif
