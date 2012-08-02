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
    COMMAND_REGISTER(commands, command_tmp, write_newline);
    COMMAND_REGISTER(commands, command_tmp, write_tab);
    COMMAND_REGISTER(commands, command_tmp, delete_before);
    COMMAND_REGISTER(commands, command_tmp, delete_after);
    COMMAND_REGISTER(commands, command_tmp, cursor_move_offset);
    COMMAND_REGISTER(commands, command_tmp, cursor_move_line);

    // Init commands
    HASH_ITER(hh, commands, command_cur, command_tmp) {
        if (command_cur->init != NULL) {
            command_cur->init(lua_state);
        }
        lua_pushcfunction(lua_state, command_cur->execute);
        lua_setglobal(lua_state, command_cur->name);
    }

    return 0;
}

int command_execute_bind(lua_State* lua_state) {

    char* keychord_str;
    char* code_str = NULL;
    int code_str_len;
    int func_ref = 0;
    keychord_command_map_t* entry;

    keychord_str = luaL_checkstring(lua_state, 1);

    if (lua_isstring(lua_state, 2)) {
        code_str = luaL_checkstring(lua_state, 2);
        code_str_len = strlen(code_str) + 1; // Plus 1 for null terminator
    } else if (lua_isfunction(lua_state, 2)) {
        luaL_checktype(lua_state, 2, LUA_TFUNCTION);
        func_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
    }

    entry = (keychord_command_map_t*)calloc(1, sizeof(keychord_command_map_t));
    snprintf(entry->keychord, sizeof(entry->keychord), "%s", keychord_str);

    if (code_str != NULL) {
        entry->lua_code = (char*)malloc(code_str_len * sizeof(char));
        snprintf(entry->lua_code, code_str_len, "%s", code_str);
    }
    if (func_ref != 0) {
        entry->lua_func_ref = func_ref;
    }

    HASH_ADD_STR(keychord_command_map, keychord, entry);

    return 0;
}

int command_execute_write(lua_State* lua_state) {

    char* str;
    int line = 0;
    int offset = 0;
    int new_line = 0;
    int new_offset = 0;
    control_t* control = control_get_active_buffer_view();

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

int command_execute_write_newline(lua_State* lua_state) {

    int line = 0;
    int offset = 0;
    int new_line = 0;
    int new_offset = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    control_get_cursor(control, &line, &offset);
    buffer_insert_str(control->buffer, line, offset, "\n", 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_write_tab(lua_State* lua_state) {
    return 0;
}

int command_execute_delete_before(lua_State* lua_state) {
    return 0;
}

int command_execute_delete_after(lua_State* lua_state) {
    return 0;
}

int command_execute_cursor_move_offset(lua_State* lua_state) {
    return 0;
}

int command_execute_cursor_move_line(lua_State* lua_state) {
    return 0;
}

int command_execute_from_keychord(char* keychord) {
    keychord_command_map_t* entry = NULL;
    HASH_FIND_STR(keychord_command_map, keychord, entry);
    if (entry != NULL) {
        if (entry->lua_func_ref != 0) {
            lua_rawgeti(lua_state, LUA_REGISTRYINDEX, entry->lua_func_ref);
            lua_pushstring(lua_state, keychord);
            lua_pcall(lua_state, 1, 0, 0);
        } else if (entry->lua_code != NULL) {
            luaL_dostring(lua_state, entry->lua_code);
        }
    }
    return 0;
}
