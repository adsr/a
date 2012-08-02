#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pcre.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ext/bstrlib/bstrlib.h"

#include "command.h"
#include "input.h"
#include "control.h"

extern lua_State* lua_state;

FILE* fdebug;

int main(int argc, char** argv) {

    fdebug = fopen("/tmp/mace.log", "a");

    char* keychord;
    char* code = (char*)malloc(64 * sizeof(char));

    command_init();
    control_init();
    input_init();

    char c;
    for (c = 0x20; c <= 0x7e; c++) {
        sprintf(code, "bind([[%c]], [[write('%c')]])", c, c);
        luaL_dostring(lua_state, code);
    }

    do {
        control_render();
        keychord = input_get_keychord(&getch);
        if (keychord == NULL) {
            continue;
        }
        command_execute_from_keychord(keychord, code);
    } while(keychord == NULL || 0 != strcmp(keychord, "q"));

    clear();
    endwin();

    return EXIT_SUCCESS;

}
