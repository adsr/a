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

    command_init();
    control_init();
    input_init();

    luaL_dofile(lua_state, "macerc");

    do {
        control_render();
        keychord = input_get_keychord(&getch);
        if (keychord == NULL) {
            continue;
        }
        control_set_status(keychord);
        if (strcmp("resize", keychord) == 0) {
            control_resize();
        } else {
            command_execute_from_keychord(keychord);
        }
    } while(keychord == NULL || 0 != strcmp(keychord, "q"));

    clear();
    endwin();

    return EXIT_SUCCESS;

}
