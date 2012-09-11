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
#include "highlighter.h"

/** @var lua_State* Lua state */
extern lua_State* lua_state;

/** @var FILE* fdebug debug file pointer */
FILE* fdebug;

/**
 * Program entry point
 *
 * @param int argc num args from command line
 * @param char** argv args from command line
 * @return int exit status
 */
int main(int argc, char** argv) {

    // Open debug log
    fdebug = fopen("/tmp/mace.log", "a");

    char* keychord;
    syntax_t* syntax_php;
    control_t* buffer_view;

    // Init sub systems
    command_init();
    control_init();
    input_init();

    // Exec macerc
    if (luaL_dofile(lua_state, "macerc") != LUA_OK) {
        endwin();
        printf("Lua error in macerc: %s\n", lua_tostring(lua_state, -1));
        return EXIT_FAILURE;
    }

    // TODO save/resume state

    // Load file into buffer
    if (argc == 2) {
        // TODO load multiple files
        buffer_view = control_get_active_buffer_view();
        HASH_FIND_STR(syntaxes, "php", syntax_php); // TODO apply syntax that matches rule in syntax_define()
        buffer_view->highlighter = highlighter_new(buffer_view->buffer, syntax_php);
        buffer_load_from_file(buffer_view->buffer, argv[1]);
    }

    // Main program loop
    do {

        // Render
        control_render();

        // Get input
        keychord = input_get_keychord(&getch);
        if (keychord == NULL) {
            continue;
        }
        control_set_status(keychord); // TODO configurable status

        // Respond to input
        if (strcmp("resize", keychord) == 0) {
            control_resize();
        } else {
            command_execute_from_keychord(keychord);
        }

    } while(keychord == NULL || 0 != strcmp(keychord, "q")); // TODO do not exit on 'q'

    // Clean up screen
    clear();
    endwin();

    return EXIT_SUCCESS;

}
