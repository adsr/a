#ifndef _COMMAND_H
#define _COMMAND_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "input.h"
#include "control.h"

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

int command_init();
int command_handle_keychord(keychord_t* keychord);

control_t* command_get_buffer_view_by_id_at_arg(lua_State *L, int argn);

int command_execute_buffer_write(lua_State* lua_state);
int command_execute_buffer_write_newline(lua_State* lua_state);
int command_execute_buffer_write_tab(lua_State* lua_state);
int command_execute_buffer_delete_before(lua_State* lua_state);
int command_execute_buffer_delete_after(lua_State* lua_state);
int command_execute_buffer_cursor_move(lua_State* lua_state);
int command_execute_buffer_cursor_home(lua_State* lua_state);
int command_execute_buffer_cursor_end(lua_State* lua_state);
int command_execute_buffer_get_prompt_id(lua_State* L);
int command_execute_buffer_get_active_id(lua_State* L);
int command_execute_buffer_set_active_id(lua_State* L);
int command_execute_buffer_get_line(lua_State* L);
int command_execute_buffer_clear(lua_State* L);
int command_execute_buffer_find_all(lua_State* L);
int command_execute_buffer_cursor_get(lua_State* L);
int command_execute_buffer_cursor_set(lua_State* L);
int command_execute_buffer_get_range(lua_State* L);
int command_execute_highlight(lua_State* L);
int command_execute_buffer_split(lua_State* L);
int command_execute_buffer_unsplit(lua_State* L);
int command_execute_syntax_define(lua_State* L);
int command_execute_syntax_add_rule(lua_State* L);
int command_execute_syntax_add_rule_multi(lua_State* L);
int command_execute_input_set_hook(lua_State* L);

#endif
