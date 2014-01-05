#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pcre.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <getopt.h>

#include "ext/bstrlib/bstrlib.h"
#include "ext/uthash/uthash.h"

#include "command.h"
#include "input.h"
#include "control.h"
#include "highlighter.h"
#include "util.h"

/** @var lua_State* Lua state */
extern lua_State* lua_state;

/** @var syntax_t* linked list of defined syntaxes */
extern syntax_t* syntaxes;

/** @var FILE* fdebug debug file pointer */
FILE* fdebug;

/** Prototypes */
int mace_init();
void usage();
void exit_fn();

/**
 * Program entry point
 *
 * @param int argc num args from command line
 * @param char** argv args from command line
 * @return int exit status
 */
int main(int argc, char** argv) {
    char* target_path;
    char* macerc_path;
    keychord_t* keychord;
    syntax_t* syntax_php;
    control_t* buffer_view;

    // Open debug log
    fdebug = fopen("/tmp/mace.log", "a");

    // Exec macerc
    mace_init(argc, argv, &macerc_path, &target_path);
    //macerc_path = "macerc";

    // Init sub systems
    command_init();
    control_init();
    atexit(exit_fn);
    input_init();

    // TODO save/resume state

    // Ensure macerc
    if (!macerc_path) {
        endwin();
        fprintf(
            stderr,
            "macerc not found in /etc ~/.mace or cwd\n"
        );
        exit(EXIT_FAILURE);
    }

    // Exec macerc
    if (luaL_dofile(lua_state, macerc_path) != LUA_OK) {
        endwin();
        fprintf(
            stderr,
            "Lua error in macerc: %s\n",
            lua_tostring(lua_state, -1)
        );
        exit(EXIT_FAILURE);
    }

    // Load file into buffer
    if (target_path) {
        // TODO load multiple files
        buffer_view = control_get_active_buffer_view();
        HASH_FIND_STR(syntaxes, "php", syntax_php); // TODO apply syntax that matches rule in syntax_define()
        buffer_view->highlighter = highlighter_new(buffer_view->buffer, syntax_php);
        buffer_load_from_file(buffer_view->buffer, target_path);
    }

    // Main program loop
    control_set_status("");
    do {

        // Render
        control_render();

        // Get input
        keychord = input_get_keychord(&getch);
        if (keychord->code_count < 1) {
            continue;
        }
        //control_set_status(keychord->name); // TODO configurable status

        // Respond to input
        if (0 == strcmp(keychord->name, "resize")) {
            control_resize(); // TODO resize doesn't work sometimes
        } else {
            command_handle_keychord(keychord);
        }

    } while(0 != strcmp(keychord->name, "q")); // TODO do not exit on 'q'

    clear();
    endwin();

    return EXIT_SUCCESS;
}

/**
 * Parses command line options
 *
 * @param int argc from main
 * @param char** argv from main
 * @param char** macerc_path path of startup script
 * @param char** target_path path of file to edit
 */
int mace_init(int argc, char** argv, char** macerc_path, char** target_path) {

    char* home_dir;
    char* home_script_path_suffix = "/.mace/macerc";
    char* etc_script_path = "/etc/macerc";
    char short_opts[] = "hs:";
    struct option long_opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "script", required_argument, NULL, 's' },
        { 0, 0, 0, 0 }
    };
    int c;
    int index;

    *macerc_path = NULL;

    // Parse command line opts
    while (1) {
        c = getopt_long(argc, argv, short_opts, long_opts, &index);
        if (-1 == c) {
            break;
        }
        switch (c) {
            case 0:
                break;
            case 'h':
                usage(stdout);
                exit(EXIT_SUCCESS);
                break;
            case 's':
                *macerc_path = strdup(optarg);
                break;
        }
    }
    if (optind < argc) {
        *target_path = argv[optind];
    }

    // Resolve macerc
    do {
        // Check command line opt
        if (*macerc_path && util_file_exists(*macerc_path)) {
            break;
        }

        // Check home dir
        home_dir = getenv("HOME");
        *macerc_path = (char*)calloc(strlen(home_dir) + strlen(home_script_path_suffix), sizeof(char));
        sprintf(*macerc_path, "%s%s", home_dir, home_script_path_suffix);
        if (util_file_exists(*macerc_path)) {
            break;
        }

        // Check etc
        *macerc_path = etc_script_path;
        if (util_file_exists(*macerc_path)) {
            break;
        }

        // Check local
        *macerc_path = "macerc";
        if (util_file_exists(*macerc_path)) {
            break;
        }

        // Not found :(
        *macerc_path = NULL;
    } while(0);

    return 0;
}

/**
 * Print usage to f
 */
void usage(FILE* f) {
    fprintf(
        f,
        "Usage: mace [OPTIONS] [FILE]\n"
        "\n"
        "Option     Long option     Description\n"
        "-h         --help          Show this help\n"
        "-s         --script        Startup script (default=~/.mace/macerc)\n"
        "\n"
    );
}

/**
 * Exit function
 */
void exit_fn() {
    if (isendwin()) {
        clear();
        endwin();
    }
}
