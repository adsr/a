#ifndef _CONTROL_H
#define _CONTROL_H

#include <ncurses.h>

#include "buffer.h"

#define CONTROL_SPLIT_TYPE_HORIZONTAL 0
#define CONTROL_SPLIT_TYPE_VERTICAL 1

typedef struct control_s {
    int width;
    int height;
    int left;
    int top;
    struct buffer_s* buffer;
    int cursor_line;
    int cursor_offset;
    int cursor_offset_target;
    int viewport_line_start;
    int viewport_line_end;
    int viewport_offset_start;
    int viewport_offset_end;
    WINDOW* window;
    WINDOW* window_line_num;
    WINDOW* window_margin_left;
    WINDOW* window_margin_right;
    int window_attrs;
    int (*resize)(struct control_s* self, int width, int height, int left, int top);
    int (*render)(struct control_s* self);
    struct control_s* buffer_view;
    struct control_s* node_child;
    struct control_s* node_sibling;
    struct control_s* node_active;
    bool is_split_container;
    int split_type;
    int split_pos;
    float split_factor;
    struct control_s* split_active;
    struct control_s* split_node_orig;
    struct control_s* split_node_new;
    int window_line_num_width;
    bool is_first_render;
} control_t;

int control_init();
int control_resize();
int control_render();
int control_render_cursor();

control_t* control_new();
control_t* control_new_multi_buffer_view();
control_t* control_new_multi_buffer_view_node();
control_t* control_new_buffer_view();

control_t* control_get_active();
control_t* control_get_active_buffer_view();
control_t* control_get_active_buffer_view_from_node(control_t* node);

int control_set_status(char* status);

int control_set_cursor(control_t* control, int line, int offset, bool set_target_offset);

int control_resize_default(control_t* self, int width, int height, int left, int top);
int control_resize_multi_buffer_view(control_t* self, int width, int height, int left, int top);
int control_resize_multi_buffer_view_node(control_t* self, int width, int height, int left, int top);
int control_resize_buffer_view(control_t* self, int width, int height, int left, int top);

int control_render_default(control_t* self);
int control_render_multi_buffer_view(control_t* self);
int control_render_multi_buffer_view_node(control_t* self);
int control_render_buffer_view(control_t* self);

int control_buffer_view_dirty_lines(control_t* self, int dirty_line_start, int dirty_line_end, bool line_count_decreased);
int control_buffer_view_render_line(control_t* self, char* line, int line_on_screen, int line_in_buffer);
int control_buffer_view_clear_line(control_t* self, int line);
int control_buffer_view_clear_lines_from(control_t* self, int clear_line_start);

#endif
