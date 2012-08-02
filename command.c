#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ext/uthash/uthash.h"

#include "command.h"
#include "buffer.h"
#include "control.h"

extern FILE* fdebug;

lua_State* lua_state;
command_t* commands = NULL;
keychord_command_map_t* keychord_command_map = NULL;

int command_init() {

    command_t* command_cur;
    command_t* command_tmp;

    // Init Lua state
    lua_state = luaL_newstate();
    luaopen_base(lua_state);
    lua_settop(lua_state, 0);

    // Register commands
    COMMAND_REGISTER(commands, command_tmp, bind);
    COMMAND_REGISTER(commands, command_tmp, write);

    // Init commands
    HASH_ITER(hh, commands, command_cur, command_tmp) {
        if (command_cur->init != NULL) {
            command_cur->init(lua_state);
        }
        lua_pushcfunction(lua_state, command_cur->execute);
        lua_setglobal(lua_state, command_cur->name);
fprintf(fdebug, "lua_setglobal[%s]\n", command_cur->name);
    }

    return 0;
}

int command_execute_bind(lua_State* lua_state) {

fprintf(fdebug, "in command_execute_bind\n");


    char* keychord_str;
    char* code_str;
    int code_str_len;
    keychord_command_map_t* entry;

    keychord_str = luaL_checkstring(lua_state, 1);
    code_str = luaL_checkstring(lua_state, 2);
    code_str_len = strlen(code_str) + 1; // Plus 1 for null terminator

    entry = (keychord_command_map_t*)calloc(1, sizeof(keychord_command_map_t));
    snprintf(entry->keychord, sizeof(entry->keychord), "%s", keychord_str);
    entry->lua_code = (char*)malloc(code_str_len * sizeof(char));
    snprintf(entry->lua_code, code_str_len, "%s", code_str);

    HASH_ADD_STR(keychord_command_map, keychord, entry);
fprintf(fdebug, "bound [%s] to [%s]\n", keychord_str, code_str);

    return 0;
}

int command_execute_write(lua_State* lua_state) {

    control_t* control;
    char* str;
    int line = 0;
    int offset = 0;
    int new_line = 0;
    int new_offset = 0;

    control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    str = luaL_checkstring(lua_state, 1);

    control_get_cursor(control, &line, &offset);
    buffer_insert_str(control->buffer, line, offset, str, 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset);

    lua_pushboolean(lua_state, TRUE);

    return 1;
}

int command_execute_from_keychord(char* keychord, char* lua_code) {
    keychord_command_map_t* entry = NULL;
    HASH_FIND_STR(keychord_command_map, keychord, entry);
    if (entry != NULL) {
        lua_code = entry->lua_code;
        luaL_dostring(lua_state, entry->lua_code);
        //luaL_dostring(lua_state, "write('a')\n"); //entry->lua_code);
    } else {
fprintf(fdebug, "did not find command for keychord=[%s]\n", keychord);
    }
    return 0;
}
