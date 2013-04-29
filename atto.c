#define _GNU_SOURCE
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
    int byte_count;
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
int buffer_clear(buffer_t* self);
char* buffer_get_content(buffer_t* self);
int buffer_destroy(buffer_t* self);
int _buffer_update_linenums(buffer_t* self, bline_t* start_line, int start_num);
int _buffer_update_srules(buffer_t* self, bline_t* line, int col, int delta, char* strdelta);
int _buffer_update_marks(buffer_t* self, bline_t* line, int col, int delta);

/**
 * Buffer line
 */
struct bline_s {
    buffer_t* buffer;
    char* content;
    int linenum;
    int length;
    sspan_t* spans;
    int style_hash;
    bline_t* next;
    bline_t* prev;
};
bline_t* bline_new(buffer_t* buffer, char* content, int n);
char* bline_get_content(bline_t* self);
char* bline_get_range(bline_t* self, int col, int length);
int bline_get_length(bline_t* self);
bline_t* bline_get_next(bline_t* self);
bline_t* bline_get_prev(bline_t* self);
int bline_insert(bline_t* self, int col, char* str, int n, bline_t** ret_line, int* ret_col);
int bline_delete(bline_t* self, int col, int num);
int bline_destroy(bline_t* self);
inline int _bline_insert_mem(bline_t* self, int col, char* str, int n);

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
bview_t* bview_get_active();
int bview_destroy(bview_t* self);

/**
 * Keymap
 */
struct keymap_s {
    kbinding_t* bindings;
    int default_fn;
    int is_fall_through_allowed;
    keymap_t* prev;
};
keymap_t* keymap_new();
int keymap_bind(keymap_t* self, char* keyc, int fn_handler);
int keymap_bind_default(keymap_t* self, int fn_handler);
int keymap_destroy(keymap_t* self);

/**
 * Keymap binding
 */
struct kbinding_s {
    char* keyc;
    int fn;
    UT_hash_handle hh;
};

/**
 * Mark
 */
struct mark_s {
    bline_t* line;
    int col;
    int tcol;
    int offset;
    mark_t* next;
    mark_t* prev;
};
mark_t* mark_new(bview_t* bview, bline_t* line, int col);
int mark_get(mark_t* self, bline_t** ret_line, int* ret_col);
int mark_set(mark_t* self, bline_t* line, int col);
int mark_move(mark_t* self, int delta);
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
    srule_t* prev;
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
};

/**
 * Hook
 */
struct hook_s {
    char* name;
    int ref;
    hook_t* next;
    hook_t* prev;
};
hook_t* hook_new(char* name, int fn_handler);
int _hook_notify(char* name);
int hook_destroy(hook_t* self);

// ========================================================= Implementation ==

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
#include "test.c"

/**
 * Globals
 */
bview_t* g_active_bview;
hook_t* hooks;

/**
 * Gets input as a keychord
 */
void _get_input(char* keyc, int* keys);

/**
 * Program entry point
 */
int main(int argc, char** argv) {
    bview_t* active_bview;
    kbinding_t* binding;
    lua_State* L;
    int luafn;
    int luaerrc;
    char* luaerr;
    char keyc[16];
    int keys[2];
    int do_exit;
    int height;
    int width;

    if (argc > 1 && !strcmp(argv[1], "-t")) {
        // Run tests
        exit(test_run());
    }

    // Init ncurses
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    curs_set(1);
    getmaxyx(stdscr, height, width);

    // Init Lua API
    L = NULL;
    lapi_init(L);
    if (luaL_dofile(L, "atto.lua") != LUA_OK) {
        endwin();
        exit(EXIT_FAILURE);
    }

    // Start loop
    do_exit = 0;
    for (;!do_exit;) {

        // Get input
        _get_input(keyc, keys);

        // Handle resize
        if (!strcmp(keyc, "resize")) {
            getmaxyx(stdscr, height, width);
            lua_pushinteger(L, width);
            lua_pushinteger(L, height);
            _hook_notify("resize");
            continue;
        }

        // Get active bview
        active_bview = bview_get_active();
        if (!active_bview || !(active_bview->keymap_stack)) {
            continue;
        }

        // Map input to Lua function
        HASH_FIND_STR(active_bview->keymap_stack->bindings, keyc, binding);
        if (binding) {
            luafn = binding->fn;
        } else if (active_bview->keymap_stack->default_fn) {
            luafn = active_bview->keymap_stack->default_fn;
        } else {
            continue;
        }

        // Invoke Lua function
        lua_rawgeti(L, LUA_REGISTRYINDEX, luafn);
        lua_createtable(L, 0, 10);
        lua_pushlightuserdata(L, active_bview->buffer);
        lua_setfield(L, -2, "buffer");
        lua_pushlightuserdata(L, active_bview);
        lua_setfield(L, -2, "bview");
        lua_pushlightuserdata(L, active_bview->cursor);
        lua_setfield(L, -2, "cursor");
        lua_pushlightuserdata(L, active_bview->cursor->line);
        lua_setfield(L, -2, "line");
        lua_pushinteger(L, active_bview->cursor->line->linenum);
        lua_setfield(L, -2, "linenum");
        lua_pushinteger(L, active_bview->cursor->col);
        lua_setfield(L, -2, "col");
        lua_pushinteger(L, active_bview->cursor->tcol);
        lua_setfield(L, -2, "tcol");
        lua_pushinteger(L, active_bview->cursor->offset);
        lua_setfield(L, -2, "offset");
        lua_pushinteger(L, width);
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, height);
        lua_setfield(L, -2, "height");
        lua_pushstring(L, keyc);
        lua_setfield(L, -2, "keychord");
        lua_pushinteger(L, keys[0]);
        lua_setfield(L, -2, "key1");
        lua_pushinteger(L, keys[1]);
        lua_setfield(L, -2, "key2");
        if ((luaerrc = lua_pcall(L, 1, 1, 0)) != 0) {
            luaerr = (char*)luaL_checkstring(L, -1);
            lua_pop(L, 1);
            lua_pushinteger(L, luaerrc);
            lua_pushstring(L, luaerr);
            _hook_notify("error");
        } else {
            do_exit = luaL_checkint(L, -1);
            lua_pop(L, 1);
        }
    }

    // End ncurses
    endwin();

    return EXIT_SUCCESS;
}

/**
 * Gets input as a keychord
 */
void _get_input(char* keyc, int* keys) {
    int ch;
    ch = getch();
    memset(keys, 0, 2);
    keys[0] = ch;
    if (ch == 10 || ch == 13) {
        keyc = "enter";
    } else if (ch == 9) {
        keyc = "tab";
    } else if (ch == 27) {
        ch = getch();
        keys[1] = ch;
        sprintf(keyc, "M%c", ch);
    } else if (ch > 0 && ch < 32) {
        sprintf(keyc, "C%c", (ch + 'A' - 1));
    } else if (ch >= 32 && ch <= 126) {
        sprintf(keyc, "%c", ch);
    } else if (ch == KEY_DOWN) {
        keyc = "down";
    } else if (ch == KEY_UP) {
        keyc = "up";
    } else if (ch == KEY_LEFT) {
        keyc = "left";
    } else if (ch == KEY_RIGHT) {
        keyc = "right";
    } else if (ch == KEY_HOME) {
        keyc = "home";
    } else if (ch == KEY_BACKSPACE) {
        keyc = "backspace";
    } else if (ch == KEY_DC) {
        keyc = "delete";
    } else if (ch == KEY_END) {
        keyc = "end";
    } else if (ch == KEY_RESIZE) {
        keyc = "resize";
    } else {
        keyc = "";
    }
}
