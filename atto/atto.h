#ifndef __ATTO_H
#define __ATTO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <math.h>
#include <pcre.h>
#include <uthash.h>
#include <utlist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termbox.h>

#include "macros.h"

/**
 * Editor concepts
 */
typedef struct editor_s editor_t; // The editor (one instance per process, houses all globals)
typedef struct buffer_s buffer_t; // A buffer of text (stored as a linked list of blines)
typedef struct bline_s bline_t; // A line of text in a buffer plus some metadata
typedef struct mark_s mark_t; // A mark (bline + char offset) in a buffer
typedef struct mark_node_s mark_node_t; // A node in a linked list of marks
typedef struct cursor_s cursor_t; // A cursor (an insertion mark and a selection bound mark) in a buffer
typedef struct srule_s srule_t; // A style rule
typedef uint_fast32_t sblock_t; // A style of a particular character
typedef struct keyinput_s keyinput_t; // A single key input (similar to a tb_event from termbox)
typedef struct keymap_s keymap_t; // A map of keychords to functions
typedef struct keymap_node_s keymap_node_t; // A node in a list of keymaps
typedef struct kbinding_s kbinding_t; // A single binding in a keymap
typedef struct keyqueue_node_s keyqueue_node_t; // A node in a queue of keys
typedef struct baction_s baction_t; // A history of buffer actions (used for undo)
typedef struct blistener_s blistener_t; // A pointer to an object plus a method to trigger on buffer events
typedef struct bview_s bview_t; // A graphical view of a buffer
typedef struct bview_rect_s bview_rect_t; // A rectangle in a bview plus default styling

/**
 * Buffer
 */
struct buffer_s {
    bline_t *first_line;
    bline_t *last_line;
    int line_count;
    int line_digits;
    int tab_stop;
    int byte_count;
    int char_count;
    mark_node_t* mark_nodes;
    srule_t* styles;
    blistener_t* listeners;
    baction_t* actions;
    baction_t* action_tail;
    baction_t* action_undone;
    char* filename;
    time_t filemtime;
    int has_unsaved_changes;
    int refcount;
    buffer_t* next;
    buffer_t* prev;
};
struct bline_s {
    buffer_t* buffer;
    char* data;
    char* data_stop;
    size_t data_len;
    size_t data_size;
    size_t line_index;
    size_t char_count;
    sblock_t* char_styles;
    int* char_widths;
    size_t width;
    mark_node_t* mark_nodes;
    srule_t* open_rule;
    bline_t* next;
    bline_t* prev;
};
struct baction_s {
    buffer_t* buffer;
    size_t line_index;
    size_t char_offset;
    size_t boffset;
    char* data;
    size_t data_len;
    int type; // ATTO_BACTION_TYPE_*
    struct {
        ssize_t byte_delta;
        ssize_t char_delta;
        ssize_t line_delta;
    } delta;
    baction_t* next;
    baction_t* prev;
    bline_t* _unsafe_start_bline;
};
typedef void (*blistener_callback_t) (
    void* listener,
    buffer_t* buffer,
    baction_t* baction
);
struct blistener_s {
    void* listener;
    blistener_callback_t callback;
    blistener_t* next;
    blistener_t* prev;
};
int buffer_new(buffer_t** ret_buffer);
int buffer_open(char* filename, int filename_len, buffer_t** ret_buffer);
int buffer_read(buffer_t* self, char* filename, int filename_len);
int buffer_write(buffer_t* self, char* filename, int filename_len, int is_append);
int buffer_set(buffer_t* self, char* data, size_t data_len);
int buffer_clear(buffer_t* self);
int buffer_insert(bline_t* self, size_t char_offset, char *data, size_t data_len, int do_record_action);
int buffer_delete(bline_t* self, size_t char_offset, size_t num_chars_to_delete, int do_record_action);
int buffer_undo(buffer_t* self);
int buffer_redo(buffer_t* self);
int buffer_repeat_at(buffer_t* self, bline_t* opt_bline, size_t opt_char_offset);
int buffer_add_style(buffer_t* self, srule_t* style);
int buffer_remove_style(buffer_t* self, srule_t* style);
int buffer_add_buffer_listener(buffer_t* self, void* listener, blistener_callback_t callback, blistener_t** ret_blistener);
int buffer_remove_buffer_listener(buffer_t* self, blistener_t* blistener);
int buffer_add_mark(bline_t* self, size_t offset, char* name, int name_len, mark_t** ret_mark);
int buffer_remove_mark(buffer_t* self, mark_t* mark);
int buffer_get_boffset(bline_t* self, size_t char_offset, size_t* ret_boffset);
int buffer_get_bline_and_offset(buffer_t* self, size_t boffset, bline_t** ret_bline, size_t* ret_offset);
int buffer_get_bline(buffer_t* self, size_t line_index, bline_t** ret_bline);
int buffer_set_tab_stop(buffer_t* self, int tab_stop);
int buffer_get_tab_stop(buffer_t* self, int* ret_tab_stop);
int buffer_sanitize_position(buffer_t* self, bline_t* opt_bline, size_t opt_offset, size_t opt_boffset, bline_t** ret_bline, size_t* ret_offset, size_t* ret_boffset);
int buffer_destroy(buffer_t* self);

/**
 * Buffer view
 */
struct bview_rect_s {
    int x;
    int y;
    int w;
    int h;
    uint16_t fg;
    uint16_t bg;
};
struct bview_s {
    int x;
    int y;
    int w;
    int h;
    int is_chromeless;
    int line_num_width;
    bview_rect_t rect_caption;
    bview_rect_t rect_lines;
    bview_rect_t rect_margin_left;
    bview_rect_t rect_buffer;
    bview_rect_t rect_margin_right;
    buffer_t* buffer;
    ssize_t viewport_x;
    ssize_t viewport_y;
    ssize_t viewport_scope_x;
    ssize_t viewport_scope_y;
    bview_t* split_parent;
    bview_t* split_child;
    float split_factor;
    int split_is_vertical;
    keymap_node_t* keymap_nodes;
    keymap_node_t* keymap_node_tail;
    cursor_t* cursors;
    cursor_t* active_cursor;
    blistener_t* blistener;
    bview_t* next;
    bview_t* prev;
};
int bview_new(char* opt_filename, int opt_filename_len, bview_t** ret_bview);
int bview_resize(bview_t* self, int x, int y, int w, int h);
int bview_open(bview_t* self, char* filename, int filename_len);
int bview_draw(bview_t* self);
int bview_get_absolute_cursor_coords(bview_t* self, int *ret_cx, int *ret_cy);
int bview_move_viewport(bview_t* self, int line_delta, int col_delta);
int bview_set_viewport(bview_t* self, bline_t* bline, size_t col);
int bview_set_viewport_scope(bview_t* self, ssize_t viewport_scope_x, ssize_t viewport_scope_y);
int bview_split(bview_t* self, int is_vertical, float factor, bview_t** ret_bview);
int bview_get_parent(bview_t* self, bview_t** ret_bview);
int bview_get_top_parent(bview_t* self, bview_t** ret_bview);
int bview_push_keymap(bview_t* self, keymap_t* keymap);
int bview_pop_keymap(bview_t* self, keymap_t** ret_keymap);
int bview_add_cursor(bview_t* self, bline_t* bline, size_t offset, int set_active, int is_asleep, cursor_t** ret_cursor);
int bview_remove_cursor(bview_t* self, cursor_t* cursor);
int bview_get_active_cursor(bview_t* self, cursor_t** ret_cursor);
int bview_set_active_cursor(bview_t* self, cursor_t* cursor);
int bview_next_active_cursor(bview_t* self, cursor_t** ret_cursor);
int bview_prev_active_cursor(bview_t* self, cursor_t** ret_cursor);
int bview_collapse_active_cursor(bview_t* self);
int bview_destroy(bview_t* self);

/**
 * Cursor
 */
struct cursor_s {
    bview_t* bview;
    mark_t* mark;
    mark_t* sel_bound;
    int is_sel_bound_anchored;
    int is_asleep;
    cursor_t* next;
    cursor_t* prev;
};
int cursor_new(bview_t* bview, bline_t* bline, size_t offset, int is_asleep, cursor_t** ret_cursor);
int cursor_anchor_sel_bound(cursor_t* self);
int cursor_retract_sel_bound(cursor_t* self);
int cursor_swap_mark_with_sel_bound(cursor_t* self);
int cursor_insert(cursor_t* self, char* str, int str_len);
int cursor_replace(cursor_t* self, char* str, int str_len);
int cursor_backspace(cursor_t* self, size_t num_chars);
int cursor_delete(cursor_t* self, size_t num_chars);
int cursor_wake_up(cursor_t* self);
int cursor_fall_asleep(cursor_t* self);
int cursor_move(cursor_t* self, ssize_t char_delta);
int cursor_move_vert(cursor_t* self, ssize_t num_lines);
int cursor_move_bol(cursor_t* self);
int cursor_move_eol(cursor_t* self);
int cursor_move_bof(cursor_t* self);
int cursor_move_eof(cursor_t* self);
int cursor_move_next_str(cursor_t* self, char* match, int match_len);
int cursor_move_prev_str(cursor_t* self, char* match, int match_len);
int cursor_move_next_rex(cursor_t* self, char* regex, int regex_len);
int cursor_move_prev_rex(cursor_t* self, char* regex, int regex_len);
int cursor_move_bracket(cursor_t* self);
int cursor_destroy(cursor_t* self);

/**
 * Mark
 */
struct mark_s {
    bline_t* bline;
    cursor_t* cursor;
    size_t char_offset;
    size_t target_char_offset;
    size_t boffset;
    char name[ATTO_MARK_MAX_NAME_LEN + 1];
    mark_node_t* _unsafe_parent_bline_mark_node;
};
struct mark_node_s {
    mark_t* mark;
    mark_node_t* next;
    mark_node_t* prev;
};
int mark_new(bline_t* bline, size_t offset, char* name, int name_len, cursor_t* opt_parent, mark_t** ret_mark);
int mark_get_char_offset(mark_t* self, size_t* ret_offset);
int mark_get_byte_offset(mark_t* self, size_t* ret_offset);
int mark_get_char_after(mark_t* self, uint32_t* ret_char);
int mark_get_char_before(mark_t* self, uint32_t* ret_char);
int mark_move(mark_t* self, ssize_t char_delta);
int mark_move_vert(mark_t* self, ssize_t num_lines);
int mark_move_bol(mark_t* self);
int mark_move_eol(mark_t* self);
int mark_move_bof(mark_t* self);
int mark_move_eof(mark_t* self);
int mark_move_next_str(mark_t* self, char* match, int match_len);
int mark_move_prev_str(mark_t* self, char* match, int match_len);
int mark_move_next_rex(mark_t* self, char* regex, int regex_len);
int mark_move_prev_rex(mark_t* self, char* regex, int regex_len);
int mark_move_bracket(mark_t* self);
int mark_find_next_str(mark_t* self, char* match, int match_len, bline_t** ret_bline, size_t* ret_offset);
int mark_find_prev_str(mark_t* self, char* match, int match_len, bline_t** ret_bline, size_t* ret_offset);
int mark_find_next_rex(mark_t* self, char* regex, int regex_len, bline_t** ret_bline, size_t* ret_offset, size_t* ret_length);
int mark_find_prev_rex(mark_t* self, char* regex, int regex_len, bline_t** ret_bline, size_t* ret_offset, size_t* ret_length);
int mark_find_bracket(mark_t* self, bline_t** ret_bline, size_t* ret_offset);
int mark_set_pos(mark_t* self, bline_t* bline, size_t char_offset);
int mark_set_boffset(mark_t* self, size_t boffset);
int mark_set_name(mark_t* self, char* name, int name_len);
int mark_destroy(mark_t* self);

/*
 * Keymap
 */
typedef ATTO_FUNCTION((*keymap_function_t));
struct keyinput_s {
    uint8_t mod;
    uint32_t ch;
    uint16_t key;
};
struct kbinding_s {
    keyinput_t keyinput;
    keymap_function_t function;
    UT_hash_handle hh;
};
struct keymap_node_s {
    keymap_t* keymap;
    keymap_node_t* next;
    keymap_node_t* prev;
};
struct keymap_s {
    kbinding_t* bindings;
    int is_fallthrough_allowed;
    keymap_function_t default_function;
};
int keymap_new(int is_fallthrough_allowed, keymap_t** ret_keymap);
int keymap_set_is_fallthrough_allowed(keymap_t* self, int is_fallthrough_allowed);
int keymap_get_is_fallthrough_allowed(keymap_t* self, int *ret_is_fallthrough_allowed);
int keymap_bind(keymap_t* self, keyinput_t keyinput, keymap_function_t function);
int keymap_unbind(keymap_t* self, keyinput_t keyinput);
int keymap_bind_default(keymap_t* self, keymap_function_t function);
int keymap_unbind_default(keymap_t* self);
int keymap_destroy(keymap_t* self);

/**
 * Style rule
 */
struct srule_s {
    char* regex;
    char* regex_end;
    pcre* cregex;
    pcre* cregex_end;
    mark_t* range_start;
    mark_t* range_end;
    sblock_t style;
    int type; // ATTO_SRULE_TYPE_*
    srule_t* next;
    srule_t* prev;
};
int srule_new_single(char* regex, int color, int bg_color, int other_attrs, srule_t** ret_srule);
int srule_new_multi(char* regex_start, char* regex_end, int color, int bg_color, int other_attrs, srule_t** ret_srule);
int srule_new_range(mark_t* start, mark_t* end, int color, int bg_color, int other_attrs, srule_t** ret_srule);
int srule_destroy(srule_t* self);

/**
 * Editor
 */
struct keyqueue_node_s {
    keyinput_t keyinput;
    keyqueue_node_t* next;
    keyqueue_node_t* prev;
};
struct editor_s {
    int width;
    int height;
    buffer_t* buffers;
    bview_t* bviews;
    bview_t* edit;
    bview_rect_t rect_status;
    bview_t* prompt;
    bview_t* active;
    keymap_t* default_keymap;
    keyqueue_node_t* keyqueue_nodes;
    keyqueue_node_t* keyqueue_node_tail;
    char* status;
    int status_len;
    int is_render_disabled;
    int should_exit;
    char* last_error;
    int last_error_size;
    FILE* fdebug;
    struct timespec tdebug;
};
extern editor_t* editor;
int editor_open_bview(char* opt_filename, int opt_filename_len, bview_t* opt_before, int make_active, bview_t** optret_bview);
int editor_close_bview(bview_t* bview);
int editor_get_num_bviews(int* ret_count);
int editor_get_bview(int bview_num, bview_t** ret_bview);
int editor_get_status(char** ret_status, int* ret_status_len);
int editor_set_status(char* status, int status_len);
int editor_set_active(bview_t* bview);
int editor_get_active(bview_t** ret_bview);
int editor_set_render_disabled(int is_render_disabled);
int editor_get_render_disabled(int* ret_is_render_disabled);
int editor_set_should_exit(int should_exit);
int editor_get_should_exit(int* ret_should_exit);

#include "util.h"

#endif
