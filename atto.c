#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
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

/**
 * =============================================================== Interface ==
 */

/**
 * Typedefs
 */
typedef struct buffer_s buffer_t; // A buffer
typedef struct bline_s bline_t; // A line in a buffer
typedef struct bview_s bview_t; // A view of a buffer
typedef struct mark_s mark_t; // A mark in a bview
typedef struct srule_s srule_t; // A style rule
typedef struct sspan_s sspan_t; // A styled span of text
typedef struct keymap_s keymap_t; // A keymap
typedef struct kbinding_s kbinding_t; // A binding in a keymap
typedef struct hook_s hook_t; // A hook

/**
 * Buffer
 */
struct buffer_s {
    int line_count;
    bline_t* cur_bline;
    bline_t* blines;
    srule_t* srules;
    mark_t* marks;
    char* content;
    int content_is_dirty;
    char* filename;
};
buffer_t* buffer_new();
int buffer_read(buffer_t* self, char* filename);
int buffer_write(buffer_t* self, char* filename, int is_append);
int buffer_insert(buffer_t* self, int line, int col, char* str);
int buffer_delete(buffer_t* self, int line, int col, int num);
int buffer_search(buffer_t* self, int line, int col, char* regex, int* ret_sl, int* ret_sc, int* ret_so, int* ret_el, int* ret_ec, int* ret_eo);
char* buffer_get_content(buffer_t* self);
char* buffer_get_range(buffer_t* self, int sl, int sc, int el, int ec);
char* buffer_get_line(buffer_t* self, int line);
int buffer_get_size(buffer_t* self);
int buffer_destroy(buffer_t* self);
bline_t* _buffer_get_line_if_valid(buffer_t* self, int line, int col);
int _buffer_refresh_srules(buffer_t* self, int line, int col);
int _buffer_refresh_marks(buffer_t* self, int line, int col);

/**
 * Buffer line
 */
struct bline_s {
    char* content;
    int length;
    sspan_t* spans;
    int style_hash;
};
bline_t* bline_new();
int bline_calc_style_hash(bline_t* self);
int bline_destroy(bline_t* self);

/**
 * Buffer view
 */
struct bview_s {
    int left;
    int top;
    int width;
    int height;
    WINDOW* win_caption;
    WINDOW* win_lines;
    WINDOW* win_margin_left;
    WINDOW* win_buffer;
    WINDOW* win_margin_right;
    WINDOW* win_split_divider;
    buffer_t* buffer;
    int viewport_x;
    int viewport_y;
    bview_t* split_child;
    float split_factor;
    int split_is_vertical;
    keymap_t* keymap_stack;
    mark_t* cursor_list;
    mark_t* cursor;
    bview_t* next;
    bview_t* prev;
};
bview_t* bview_new();
int bview_update(bview_t* self);
int bview_move(bview_t* self, int x, int y);
int bview_resize(bview_t* self, int w, int h);
int bview_viewport_move(bview_t* self, int line_delta, int col_delta);
int bview_viewport_set(bview_t* self, int line, int col);
bview_t* bview_split(bview_t* self, int is_vertical, float factor);
int bview_keymap_push(bview_t* self, keymap_t* keymap);
keymap_t* bview_keymap_pop(bview_t* self);
int bview_set_active(bview_t* self);
int bview_destroy(bview_t* self);

/**
 * Mark
 */
struct mark_s {
    buffer_t* buffer;
    bline_t* line;
    int col;
    int offset;
};
mark_t* mark_new(buffer_t* buffer);
int mark_add(bview_t* bview, int line, int col);
int mark_get(mark_t* self, int* line, int* col);
int mark_set(mark_t* self, int line, int col);
int mark_destroy(mark_t* self);

/**
 * Style rule
 */
struct srule_s {
    char* regex_start;
    char* regex_end;
    mark_t* start;
    mark_t* end;
    int is_bold;
    int color;
    srule_t* next;
};
srule_t* srule_new();
srule_t* srule_add_single(char* regex);
srule_t* srule_add_multi(char* regex_start, char* regex_end);
srule_t* srule_add_range(mark_t* start, mark_t* end);
int srule_destroy(srule_t* self);

/**
 * Styled span
 */
struct sspan_s {
    int col;
    int length;
    int style;
    sspan_t* next;
} sspan_t;

/**
 * Keymap
 */
struct keymap_s {
    kbinding_t* bindings;
    int default_ref;
};
keymap_t* keymap_new();
int keymap_bind(keymap_t* self, char* keychord, int ref);
int keymap_bind_default(keymap_t* self, int ref);
int keymap_destroy(keymap_t* self);

/**
 * Keymap binding
 */
struct kbinding_s {
    char* keychord;
    int ref;
    UT_hash_handle hh;
} kbinding_t;

/**
 * Hook
 */
struct hook_s {
    char* name;
    int ref;
    hook_t* next;
    hook_t* prev;
};
hook_t* hook_new(char* name, int ref);
int hook_notify(char* name);
int hook_destroy(hook_t* self);

/**
 * Lua bindings
 */
int lapi_init(lua_State* L);

#include "buffer.c"
#include "bline.c"
#include "bview.c"
#include "mark.c"
#include "srule.c"
#include "sspan.c"
#include "keymap.c"
#include "kbinding.c"
#include "hook.c"
#include "lapi.c"

int main (int argc, char** argv) {
    // load lua script
    // get input
    // get ref from active bview's keymap
    // call ref
    lua_State* L;

    lapi_init(L);
    if (luaL_dofile(L, "") != LUA_OK) {
        
    }

    return 0;
}
