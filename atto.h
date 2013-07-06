#ifndef __ATTO_H
#define __ATTO_H

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>
#include <pcre.h>
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include "ext/uthash/uthash.h"
#include "ext/uthash/utarray.h"
#include "ext/uthash/utlist.h"

#define ATTO_RC_OK 0
#define ATTO_RC_ERR 1
#define ATTO_KEYS_LEN 2
#define ATTO_KEYC_LEN 16
#define ATTO_SRULE_TYPE_SINGLE 0
#define ATTO_SRULE_TYPE_MULTI 1
#define ATTO_SRULE_TYPE_RANGE 2
#define ATTO_BUFFER_DATA_ALLOC_INCR 1024
#define ATTO_LINE_OFFSET_ALLOC_INCR 64
#define ATTO_SSPAN_RANGE_ALLOC_INCR 10
#define ATTO_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define ATTO_MAX(a,b) (((a) > (b)) ? (a) : (b))

/**
 * Typedefs
 */
typedef struct buffer_s buffer_t; // A buffer of text
typedef struct bline_s bline_t; // Metadata about a line of text in a buffer
typedef struct bview_s bview_t; // A view of a buffer
typedef struct blistener_s blistener_t; // An object that listener to a buffer
typedef void (*blistener_callback_t) ( // A pointer to a function be invoked on a blistener
    buffer_t* buffer,
    void* listener,
    int line,
    int col,
    char* delta,
    int delta_len
);
typedef struct mark_s mark_t; // A mark in a buffer
typedef struct srule_s srule_t; // A style rule
typedef struct srule_node_s srule_node_t; // A node in a list of styles
typedef struct sspan_s sspan_t; // A styled span of text
typedef struct keymap_s keymap_t; // A map of inputs to functions
typedef struct keymap_node_s keymap_node_t; // A node in a list of keymaps
typedef struct kbinding_s kbinding_t; // A single binding in a keymap
typedef struct hook_s hook_t; // An event hook

/**
 * Buffer
 */
struct buffer_s {
    char* data;
    int data_size;
    char* filename;
    time_t filemtime;
    bline_t* blines;
    int blines_size;
    mark_t* marks;
    srule_node_t* styles;
    blistener_t* listeners;
    int has_unsaved_changes;
    int line_count;
    int byte_count;
};
buffer_t* buffer_new();
int buffer_read(buffer_t* self, char* filename);
int buffer_write(buffer_t* self, char* filename, int is_append);
int buffer_clear(buffer_t* self);
int buffer_set(buffer_t* self, char* data, int len);
int buffer_insert(buffer_t* self, int offset, char* str, int len, int* ret_offset, int* ret_line, int* ret_col);
int buffer_delete(buffer_t* self, int offset, int len);
int buffer_get_line(buffer_t* self, int line, int from_col, char* usebuf, int usebuf_len, char** ret_line, int* ret_len);
int buffer_get_line_offset_len(buffer_t* self, int line, int from_col, int* ret_offset, int* ret_len);
int buffer_get_substr(buffer_t* self, int offset, int len, char* usebuf, int usebuf_len, char** ret_substr, int* ret_len);
int buffer_get_line_col(buffer_t* self, int offset, int* ret_line, int* ret_col);
int buffer_get_offset(buffer_t* self, int line, int col);
int buffer_search(buffer_t* self, char* needle, int offset);
int buffer_search_reverse(buffer_t* self, char* needle, int offset);
int buffer_regex(buffer_t* self, char* regex, int offset, int length);
int _buffer_search(buffer_t* self, char* needle, int offset, int is_reverse);
int buffer_add_style(buffer_t* self, srule_t* style);
int buffer_remove_style(buffer_t* self, srule_t* style);
mark_t* buffer_add_mark(buffer_t* self, int offset);
int buffer_remove_mark(buffer_t* self, mark_t* mark);
blistener_t* buffer_add_buffer_listener(buffer_t* self, void* listener, blistener_callback_t fn);
int buffer_remove_buffer_listener(buffer_t* self, blistener_t* blistener);
int buffer_destroy(buffer_t* self);
int _buffer_update_metadata(buffer_t* self, int offset, int line, int col, char* delta, int delta_len);
int _buffer_update_line_offsets(buffer_t* self, int dirty_line, char* delta, int delta_len);
int _buffer_update_marks(buffer_t* self, int offset, int delta);
int _buffer_update_styles(buffer_t* self, int dirty_line, char* delta, int delta_len, int bail_on_matching_style_hash);
int _buffer_expand_line_structs(buffer_t* self);

/**
 * Buffer line
 */
struct bline_s {
    int offset;
    int length;
    int style_hash;
    sspan_t* sspans;
    int sspans_len;
    int sspans_size;
    srule_t* open_rule;
};

/**
 * Buffer listener
 */
struct blistener_s {
    void* listener;
    blistener_callback_t fn;
    blistener_t* next;
};

/**
 * Buffer view
 */
struct bview_s {
    int left;
    int top;
    int width;
    int height;
    int is_chromeless;
    WINDOW* win_caption;
    WINDOW* win_lines;
    WINDOW* win_margin_left;
    WINDOW* win_buffer;
    WINDOW* win_margin_right;
    int lines_width;
    buffer_t* buffer;
    int viewport_x;
    int viewport_y;
    int viewport_w;
    int viewport_h;
    int viewport_scope;
    bview_t* split_parent;
    bview_t* split_child;
    float split_factor;
    int split_is_vertical;
    keymap_node_t* keymaps;
    keymap_node_t* keymap_tail;
    mark_t* cursor_list;
    mark_t* cursor;
    bview_t* next;
    bview_t* prev;
};
bview_t* bview_new(buffer_t* opt_buffer, int is_chromeless);
int bview_update(bview_t* self);
int bview_update_cursor(bview_t* self);
int bview_resize(bview_t* self, int x, int y, int w, int h);
int bview_viewport_move(bview_t* self, int line_delta, int col_delta);
int bview_viewport_set(bview_t* self, int line, int col);
int bview_viewport_set_scope(bview_t* self, int viewport_scope);
int _bview_update_viewport(bview_t* self, int line, int col);
int _bview_update_viewport_dimension(bview_t* self, int line, int viewport_h, int* viewport_y);
bview_t* bview_split(bview_t* self, int is_vertical, float factor);
bview_t* bview_get_parent(bview_t* self);
bview_t* bview_get_top_parent(bview_t* self);
int bview_keymap_push(bview_t* self, keymap_t* keymap);
keymap_t* bview_keymap_pop(bview_t* self);
int bview_set_active(bview_t* self);
bview_t* bview_get_active();
int bview_destroy(bview_t* self);
void _bview_buffer_callback(buffer_t* buffer, void* listener, int line, int col, char* delta, int delta_len);
int bview_set_prompt_label(char* label);
char* bview_get_prompt_label();

/**
 * Keymap
 */
struct keymap_s {
    kbinding_t* bindings;
    int default_fn;
    int is_fall_through_allowed;
};
struct keymap_node_s {
    keymap_t* keymap;
    keymap_node_t* next;
    keymap_node_t* prev;
};
keymap_t* keymap_new(int is_fall_through_allowed);
int keymap_bind(keymap_t* self, char* keyc, int fn_handler);
int keymap_bind_default(keymap_t* self, int fn_handler);
int keymap_destroy(keymap_t* self);

/**
 * Keymap binding
 */
struct kbinding_s {
    char keyc[ATTO_KEYC_LEN];
    int fn;
    UT_hash_handle hh;
};
kbinding_t* kbinding_new(char* keyc, int fn);
int kbinding_destroy(kbinding_t* self);

/**
 * Mark
 */
struct mark_s {
    buffer_t* buffer;
    int line;
    int col;
    int target_col;
    int offset;
    mark_t* next;
};
int mark_get(mark_t* self, int* ret_line, int* ret_col);
int mark_set(mark_t* self, int offset);
int mark_set_line_col(mark_t* self, int line, int col);
int mark_move(mark_t* self, int delta);
int mark_move_line(mark_t* self, int line_delta);

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
    int color;
    int bg_color;
    int other_attrs;
    int attrs;
    int type; // ATTO_SRULE_TYPE_*
};
struct srule_node_s {
    srule_t* rule;
    srule_node_t* next;
};
srule_t* srule_new_single(char* regex, int color, int bg_color, int other_attrs);
srule_t* srule_new_multi(char* regex_start, char* regex_end, int color, int bg_color, int other_attrs);
srule_t* srule_new_range(mark_t* start, mark_t* end, int color, int bg_color, int other_attrs);
srule_t* _srule_new(int color, int bg_color, int other_attrs);
int _srule_destroy(srule_t* self);

/**
 * Styled span
 */
struct sspan_s {
    int length;
    int attrs;
};

/**
 * Hook
 */
struct hook_s {
    char* name;
    int ref;
    hook_t* next;
};
hook_t* hook_new(char* name, int fn_handler);
int _hook_notify(char* name, int num_args);
int hook_destroy(hook_t* self);

/**
 * Layout
 */
int _layout_init(lua_State* L, buffer_t* buffer);
int _layout_resize(int width, int height);

/**
 * Script
 */
int _script_init(lua_State* L);

/**
 * Lua API
 */
int lapi_init(lua_State** L);

/**
 * Util functions
 */
const char* util_memrmem(const char* s, size_t slen, const char* t, size_t tlen);
pcre* util_compile_regex(char* regex);
int util_get_ncurses_color_pair(int fg_num, int bg_num);
int util_file_exists(const char *path);

/**
 * Helper functions for main
 */
void _main_init_ncurses(int* width, int* height);
void _main_run_lua_script(lua_State* L);
void _main_loop(lua_State* L, int width, int height);
void _main_get_input(char* keyc, int* keys);
void _main_handle_resize(lua_State* L, int* width, int* height);
int _main_map_input_to_function(bview_t* bview, char* keyc);
int _main_invoke_function(int luafn, bview_t* bview, lua_State* L, int width, int height, char* keyc, int* keys);
int _main_lapi_debug(lua_State* L);

/**
 * Test suite
 */
int test_run();

/**
 * Globals
 */
extern bview_t* g_bview_edit;
extern bview_t* g_bview_status;
extern bview_t* g_bview_prompt;
extern bview_t* g_bview_active;
extern hook_t* g_hooks;
extern WINDOW* g_prompt_label;
extern char* g_prompt_label_str;
extern int g_prompt_label_len;
extern int g_width;
extern int g_height;

/**
 * Debug
 */
extern FILE* fdebug;
extern struct timespec tdebug;
#define ATTO_DEBUG 1
#define ATTO_DEBUG_PRINTF(fmt, ...) \
    do { \
        if (ATTO_DEBUG) { \
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tdebug); \
            fprintf(fdebug, "%ld [%s] ", tdebug.tv_nsec, __PRETTY_FUNCTION__); \
            fprintf(fdebug, fmt, __VA_ARGS__); \
        } \
    } while (0)

#endif
