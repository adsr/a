#include "atto.h"

/**
 * Init some Lua globals
 */
int _script_init(lua_State* L) {
    short color_default_bg;
    short color_default_fg;

    // Init ncurses constants
    pair_content(0, &color_default_fg, &color_default_bg);
    lua_pushinteger(L, COLOR_BLACK);
    lua_setglobal(L, "COLOR_BLACK");
    lua_pushinteger(L, COLOR_RED);
    lua_setglobal(L, "COLOR_RED");
    lua_pushinteger(L, COLOR_GREEN);
    lua_setglobal(L, "COLOR_GREEN");
    lua_pushinteger(L, COLOR_YELLOW);
    lua_setglobal(L, "COLOR_YELLOW");
    lua_pushinteger(L, COLOR_BLUE);
    lua_setglobal(L, "COLOR_BLUE");
    lua_pushinteger(L, COLOR_MAGENTA);
    lua_setglobal(L, "COLOR_MAGENTA");
    lua_pushinteger(L, COLOR_CYAN);
    lua_setglobal(L, "COLOR_CYAN");
    lua_pushinteger(L, COLOR_WHITE);
    lua_setglobal(L, "COLOR_WHITE");
    lua_pushinteger(L, color_default_bg);
    lua_setglobal(L, "COLOR_DEFAULT_BG");
    lua_pushinteger(L, color_default_fg);
    lua_setglobal(L, "COLOR_DEFAULT_FG");
    lua_pushinteger(L, A_NORMAL);
    lua_setglobal(L, "A_NORMAL");
    lua_pushinteger(L, A_STANDOUT);
    lua_setglobal(L, "A_STANDOUT");
    lua_pushinteger(L, A_UNDERLINE);
    lua_setglobal(L, "A_UNDERLINE");
    lua_pushinteger(L, A_REVERSE);
    lua_setglobal(L, "A_REVERSE");
    lua_pushinteger(L, A_BLINK);
    lua_setglobal(L, "A_BLINK");
    lua_pushinteger(L, A_DIM);
    lua_setglobal(L, "A_DIM");
    lua_pushinteger(L, A_BOLD);
    lua_setglobal(L, "A_BOLD");

    return ATTO_RC_OK;
}
