#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "control.h"

extern FILE* fdebug;

buffer_t* buffer_new() {
    int zero = 0;
    buffer_t* buffer = (buffer_t*)calloc(1, sizeof(buffer_t));
    buffer->buffer = bfromcstr("");
    buffer->char_count = 0;
    buffer->line_count = 1;
    utarray_new(buffer->line_offsets, &ut_int_icd);
    utarray_push_back(buffer->line_offsets, &zero); // 0 -> 0
    buffer->buffer_listener_head = NULL;
    return buffer;
}

int buffer_insert_str(
    buffer_t* buffer,
    int line,
    int offset,
    const char* str,
    int chars_to_delete,
    int* new_line,
    int* new_offset
) {

    int pre_line_count = buffer->line_count;
    int buffer_offset = buffer_get_buffer_offset(buffer, line, offset);
    int new_buffer_offset;
    bstring insert_str = bfromcstr(str);
    int chars_to_insert = blength(insert_str);
    int line_delta;

    if (chars_to_delete > 0) {
        // Delete chars_to_delete chars at buffer_offset
        bdelete(buffer->buffer, buffer_offset, chars_to_delete);
    }
    // Insert insert_str at buffer_offset
    binsert(buffer->buffer, buffer_offset, insert_str, ' ');

    // Update char_count
    buffer->char_count = buffer->char_count + chars_to_insert - chars_to_delete;
    if (buffer->char_count < 0) {
        buffer->char_count = 0;
    }

    // Calculate line offsets
    buffer_calc_line_offsets(buffer);

    // Calculate new_buffer_offset
    new_buffer_offset = buffer_offset + chars_to_insert;
    //if (new_buffer_offset < 0) {
    //    new_buffer_offset = 0;
    //}

    // Calculate line and offset of new_buffer_offset
    buffer_get_line_and_offset(buffer, new_buffer_offset, new_line, new_offset);

    // Flag lines as dirty
    line_delta = buffer->line_count - pre_line_count;
    if (line_delta == 0) {
        buffer_dirty_lines(
            buffer,
            (line < *new_line ? line : *new_line),
            (line > *new_line ? line : *new_line),
            line_delta
        );
    } else { // line_delta != 0 (line count changed)
        buffer_dirty_lines(
            buffer,
            (line < *new_line ? line : *new_line),
            buffer->line_count - 1,
            line_delta
        );
    }

    return 0;
}

int buffer_add_listener(buffer_t* buffer, void (*on_dirty_lines)(buffer_t* buffer, void* listener, int line_start, int line_end, int line_delta), void* listener) {
    buffer_listener_t* node = (buffer_listener_t*)calloc(1, sizeof(buffer_listener_t));
    buffer_listener_t* cur_node;
    node->listener = listener;
    node->on_dirty_lines = on_dirty_lines;
    node->next = NULL;
    if (buffer->buffer_listener_head == NULL) {
        buffer->buffer_listener_head = node;
    } else {
        cur_node = buffer->buffer_listener_head;
        while (cur_node->next != NULL) {
            cur_node = cur_node->next;
        }
        cur_node->next = node;
    }
    return 0;
}

int buffer_get_buffer_offset(buffer_t* buffer, int line, int offset) {

    int buffer_offset;
    int* line_offset = NULL;
    int last_offset = buffer->char_count;// - 1;
    //if (last_offset < 0) {
    //    last_offset = 0;
    //}

    if (line < 0) {
        // Return 0 for line < 0
        return 0;
    } else if (line == 0) {
        // For line 0, return offset (clamped to valid bounds)
        return offset < 0 ? 0 : (offset > last_offset ? last_offset : offset);
    } else if (line >= buffer->line_count) {
        // Return last_offset for line >= line_count
        return last_offset;
    }

    line_offset = (int*)utarray_eltptr(buffer->line_offsets, line);

    buffer_offset = *line_offset + offset; // offset of line + offset
    if (buffer_offset > last_offset) {
        // Return last_offset for buffer_offset > last_offset
        return last_offset;
    }
    if (buffer_offset < 0) {
        // Return 0 for buffer_offset < 0
        return 0;
    }
    return buffer_offset;
}

int buffer_calc_line_offsets(buffer_t* buffer) {

    int look_offset = 0;
    int newline_offset = 0;
    int line_count = 0;
    int* line_offset_elem;
    bstring newline = bfromcstr("\n");

    while (1) {
        line_count += 1;
        // Find next newline
        newline_offset = binstr(buffer->buffer, look_offset, newline);
        if (newline_offset == BSTR_ERR) {
            // No more newlines; break
            break;
        }
        newline_offset += 1; // Increment to get to first char of line

        if (utarray_len(buffer->line_offsets) < line_count + 1) {
            // Resize array if necessary
            utarray_resize(buffer->line_offsets, line_count + 1);
        }

        // Let buffer->line_offsets[line_count - 1] = newline_offset
        line_offset_elem = (int*)utarray_eltptr(buffer->line_offsets, line_count);
        *line_offset_elem = newline_offset;

        // Advance look_offset
        look_offset = newline_offset;
    }

    // Update line_count and char_count
	buffer->line_count = line_count;
    buffer->char_count = blength(buffer->buffer);

    return 0;
}

int buffer_dirty_lines(buffer_t* buffer, int line_start, int line_end, int line_delta) {
    buffer_listener_t* cur_node;
    cur_node = buffer->buffer_listener_head;
    while (cur_node) {
        cur_node->on_dirty_lines(
            buffer,
            cur_node->listener,
            line_start,
            line_end,
            line_delta
        );
        cur_node = cur_node->next;
    }
    return 0;
}

int buffer_get_line_and_offset(buffer_t* buffer, int buffer_offset, int* line, int* offset) {

    int prev_start_offset = 0;
    int cur_line = 0;
    int last_line_length;
    int* start_offset;
    int* last_line_offset;

    if (buffer_offset < 1) {
        // buffer_offset < 1 resolves to line=0,offset=0
        *line = 0;
        *offset = 0;
        return 0;
    }

    // Loop through line offsets
    for (
        start_offset = (int*)utarray_front(buffer->line_offsets);
        start_offset != NULL;
        start_offset = (int*)utarray_next(buffer->line_offsets, start_offset)
    ) {
        if (*start_offset == buffer_offset) {
            // buffer_offset is at start of cur_line
            *line = cur_line;
            *offset = 0;
            return 0;
        } else if (*start_offset > buffer_offset) {
            // buffer_offset is at some offset on cur_line
            *line = cur_line - 1;
            if (*line < 0) {
                *line = 0;
            }
            *offset = buffer_offset - prev_start_offset;
            if (*offset < 0) {
                *offset = 0;
            }
            return 0;
        }
        // buffer_offset is on some other line
        prev_start_offset = *start_offset;
        cur_line += 1;
    }

    // If we get here, buffer_offset is somewhere on the last line
    *line = buffer->line_count - 1;
    if (*line < 0) {
        *line = 0;
    }
    last_line_offset = (int*)utarray_eltptr(buffer->line_offsets, buffer->line_count - 1);
    last_line_length = buffer->char_count - *last_line_offset;
    *offset = buffer_offset - prev_start_offset;
    if (last_line_length < *offset) {
        *offset = last_line_length;
    }
    if (*offset < 0) {
        *offset = 0;
    }
	return 0;

}

char** buffer_get_lines(buffer_t* buffer, int dirty_line_start, int dirty_line_end, int* line_count) {

    int line;
    char** lines;

    *line_count = 0;

    int line_count_max = dirty_line_end - dirty_line_start + 1;
    if (line_count_max < 1) {
        return NULL;
    }

    lines = (char**)malloc(line_count_max * sizeof(char*));
    for (line = dirty_line_start; line <= dirty_line_end; line++) {
        *(lines + *line_count) = buffer_get_line(buffer, line);
        if (*(lines + *line_count) == NULL) {
            break;
        }
        *line_count += 1;
    }

    return lines;

}

char* buffer_get_line(buffer_t* buffer, int line) {

    int start_offset = 0;
    int length = 0;
    bstring line_str;

    if (line < 0 || line >= buffer->line_count) {
        return NULL;
    }

    buffer_get_line_offset_and_length(buffer, line, &start_offset, &length);
    line_str = bmidstr(buffer->buffer, start_offset, length);

    return line_str != NULL ? (char*)line_str->data : NULL;

}

int buffer_get_line_offset_and_length(buffer_t* buffer, int line, int* start_offset, int* length) {

    int end_offset; // Inlcusive end offset in buffer->buffer to return

    if (line < 0 || line >= buffer->line_count) {
        return 0;
    }

    *start_offset = buffer_get_buffer_offset(buffer, line, 0);
    if (line + 1 < buffer->line_count) {
        // Minus 1 is the previous newline
        // Minus 2 is the last char of the previous line
        end_offset = buffer_get_buffer_offset(buffer, line + 1, 0) - 2;
    } else {
        end_offset = buffer->char_count - 1; // Last char offset of buffer
    }

    *length = end_offset - *start_offset + 1;

    return 0;

}

int buffer_load_from_file(buffer_t* buffer, char* filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        return 0;
    }
    buffer->buffer = bread((bNread)fread, fp);
    buffer_calc_line_offsets(buffer);
    return 0;
}
