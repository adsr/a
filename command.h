#ifndef _COMMAND_H
#define _COMMAND_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ext/uthash/uthash.h"

#define COMMAND_REGISTER(CMDS, TMP, CMD) \
    do { \
        TMP = (command_t*)calloc(1, sizeof(command_t)); \
        strcpy(TMP->name, #CMD); \
        TMP->execute = command_execute_ ## CMD; \
        HASH_ADD_STR(CMDS, name, TMP); \
    } while (0)

typedef struct command_s {
    char name[64];
    int (*init)(lua_State *lua_state);
    int (*execute)(lua_State *lua_state);
    UT_hash_handle hh;
} command_t;

typedef struct keychord_command_map_s {
    char keychord[64];
    char* lua_code;
    int lua_func_ref;
    UT_hash_handle hh;
} keychord_command_map_t;

int command_init();
int command_execute_from_keychord(char* keychord);

int command_execute_write(lua_State* lua_state);
int command_execute_bind(lua_State* lua_state);
int command_execute_write_newline(lua_State* lua_state);
int command_execute_write_tab(lua_State* lua_state);
int command_execute_delete_before(lua_State* lua_state);
int command_execute_delete_after(lua_State* lua_state);
int command_execute_cursor_move_offset(lua_State* lua_state);
int command_execute_cursor_move_line(lua_State* lua_state);


#endif
