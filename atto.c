#include "atto.h"

/**
 * Globals
 */
bview_t* g_bview_edit = NULL;
bview_t* g_bview_status = NULL;
bview_t* g_bview_prompt = NULL;
bview_t* g_bview_active = NULL;
hook_t* g_hooks = NULL;
FILE* fdebug = NULL;
struct timespec tdebug;

/**
 * Program entry point
 */
int main(int argc, char** argv) {
    lua_State* L;
    int width;
    int height;
    buffer_t* buffer;

    // Open debug file
    if (ATTO_DEBUG) {
        fdebug = fopen("debug.log", "w");
    }

    // Init ncurses
    ATTO_DEBUG_PRINTF("%s\n", "Init ncurses");
    _main_init_ncurses(&width, &height);

    // Init Lua API
    ATTO_DEBUG_PRINTF("%s\n", "Init Lua API");
    L = NULL;
    lapi_init(&L);
    if (ATTO_DEBUG) {
        lua_pushcfunction(L, _main_lapi_debug);
        lua_setglobal(L, "atto_debug");
    }

    // Init layout
    ATTO_DEBUG_PRINTF("%s\n", "Init layout");
    buffer = NULL; // TODO parse args
    refresh();
    _layout_init(L, buffer);
    g_bview_active = g_bview_edit;

    // Show screen once
    ATTO_DEBUG_PRINTF("%s\n", "Resize and render layout");
    _layout_resize(width, height);
    bview_update(g_bview_active);
    doupdate();

    // Run Lua user script
    ATTO_DEBUG_PRINTF("%s\n", "Run Lua script");
    _main_run_lua_script(L);

    // Enter main loop
    ATTO_DEBUG_PRINTF("%s\n", "Enter main loop");
    _main_loop(L, width, height);

    // End ncurses
    ATTO_DEBUG_PRINTF("%s\n", "End ncurses");
    endwin();

    return EXIT_SUCCESS;
}

void _main_loop(lua_State* L, int width, int height) {
    int do_exit;
    char* keyc;
    int keys[2];
    int luafn;

    // Start loop
    keyc = (char*)calloc(ATTO_KEYC_LEN + 1, sizeof(char));
    do_exit = 0;
    for (;!do_exit;) {

        // Update screen
        doupdate();

        // Get input
        _main_get_input(keyc, keys);
        ATTO_DEBUG_PRINTF("Got input: %s (%d %d)\n", keyc, keys[0], keys[1]);
        if (!strcmp(keyc, "q")) {
            break;
        }

        // Handle resize
        if (!strcmp(keyc, "resize")) {
            _main_handle_resize(L, &width, &height);
            continue;
        }

        // Map input to function
        luafn = _main_map_input_to_function(g_bview_active, keyc);
        ATTO_DEBUG_PRINTF("Function for input: %d\n", luafn);
        if (luafn == LUA_REFNIL) {
            continue;
        }

        // Invoke function
        do_exit = _main_invoke_function(luafn, g_bview_active, L, width, height, keyc, keys);
        do_exit = 0; // TODO remove
    }

    free(keyc);
}

void _main_get_input(char* keyc, int* keys) {
    int ch;
    ch = getch();
    memset(keys, 0, 2);
    keys[0] = ch;
    if (ch == 10 || ch == 13) {
        sprintf(keyc, "%s", "enter");
    } else if (ch == 9) {
        sprintf(keyc, "%s", "tab");
    } else if (ch == 27) {
        ch = getch();
        keys[1] = ch;
        sprintf(keyc, "M%c", ch);
    } else if (ch > 0 && ch < 32) {
        sprintf(keyc, "C%c", (ch + 'A' - 1));
    } else if (ch >= 32 && ch <= 126) {
        sprintf(keyc, "%c", ch);
    } else if (ch == KEY_DOWN) {
        sprintf(keyc, "%s", "down");
    } else if (ch == KEY_UP) {
        sprintf(keyc, "%s", "up");
    } else if (ch == KEY_LEFT) {
        sprintf(keyc, "%s", "left");
    } else if (ch == KEY_RIGHT) {
        sprintf(keyc, "%s", "right");
    } else if (ch == KEY_HOME) {
        sprintf(keyc, "%s", "home");
    } else if (ch == KEY_BACKSPACE) {
        sprintf(keyc, "%s", "backspace");
    } else if (ch == KEY_DC) {
        sprintf(keyc, "%s", "delete");
    } else if (ch == KEY_END) {
        sprintf(keyc, "%s", "end");
    } else if (ch == KEY_RESIZE) {
        sprintf(keyc, "%s", "resize");
    } else {
        sprintf(keyc, "%s", "");
    }
}

void _main_init_ncurses(int* width, int* height) {
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    curs_set(1);
    getmaxyx(stdscr, *height, *width);
}

void _main_run_lua_script(lua_State* L) {
    if (luaL_dofile(L, "atto.lua") != LUA_OK) {
        endwin();
        printf("Error in lua script: %s\n", lua_tostring(L, -1));
        exit(EXIT_FAILURE);
    }
}

void _main_handle_resize(lua_State* L, int* width, int* height) {
    getmaxyx(stdscr, *height, *width);
    _layout_resize(*width, *height);
    lua_pushinteger(L, *width);
    lua_pushinteger(L, *height);
}

int _main_map_input_to_function(bview_t* bview, char* keyc) {
    kbinding_t* binding;
    keymap_node_t* node;
    keymap_t* keymap;
    for (node = bview->keymap_tail; node; node = node->prev) {
        keymap = node->keymap;
        HASH_FIND_STR(keymap->bindings, keyc, binding);
        if (binding) {
            return binding->fn;
        } else if (keymap->default_fn) {
            return keymap->default_fn;
        } else if (keymap->is_fall_through_allowed) {
            continue;
        }
        break;
    }
    return LUA_REFNIL;
}

int _main_invoke_function(int luafn, bview_t* bview, lua_State* L, int width, int height, char* keyc, int* keys) {
    int luaerrc;
    char* luaerr;
    int do_exit;
    int num_args;

    lua_rawgeti(L, LUA_REGISTRYINDEX, luafn);
    lua_createtable(L, 0, 10);

    lua_pushlightuserdata(L, bview->buffer);
    lua_setfield(L, -2, "buffer");

    lua_pushlightuserdata(L, bview);
    lua_setfield(L, -2, "bview");

    lua_pushlightuserdata(L, bview->cursor);
    lua_setfield(L, -2, "cursor");

    lua_pushinteger(L, bview->cursor->line);
    lua_setfield(L, -2, "line");

    lua_pushinteger(L, bview->cursor->col);
    lua_setfield(L, -2, "col");

    lua_pushinteger(L, bview->cursor->target_col);
    lua_setfield(L, -2, "target_col");

    lua_pushinteger(L, bview->cursor->offset);
    lua_setfield(L, -2, "offset");

    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");

    lua_pushstring(L, keyc);
    lua_setfield(L, -2, "keyc");

    lua_pushinteger(L, keys[0]);
    lua_setfield(L, -2, "key1");

    lua_pushinteger(L, keys[1]);
    lua_setfield(L, -2, "key2");

    num_args = lua_gettop(L);
    if ((luaerrc = lua_pcall(L, 1, 1, 0)) != 0) {
        luaerr = (char*)luaL_checkstring(L, -1);
        endwin();
        printf("pcall error: %s\n", luaerr);
        exit(1);
        // TODO fix error handling
        /*
        lua_pop(L, 1);
        lua_pushinteger(L, luaerrc);
        lua_pushstring(L, luaerr);
        _hook_notify("error", 2);
        */
    } else {
        num_args = lua_gettop(L) - num_args;
        if (num_args > 1) {
            lua_pop(L, num_args);
            do_exit = 1;
        }
        do_exit = 0;
    }
    return do_exit;
}

int _main_lapi_debug(lua_State* L) {
    int retval;
    char* msg;
    msg = (char*)luaL_checkstring(L, 1);
    ATTO_DEBUG_PRINTF("Lua: %s\n", msg);
    retval = ATTO_RC_OK;
    lua_pushinteger(L, retval);
    return 1;
}
