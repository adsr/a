#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pcre.h>

#include "ext/uthash/uthash.h"

#include "command.h"
#include "buffer.h"
#include "control.h"
#include "highlighter.h"
#include "util.h"

extern FILE* fdebug;

/**
 * test multi
 * line highlighting
 * /* wee
 * include
 */ int test; /* tricky multi line */

lua_State* lua_state;
command_t* commands = NULL;
keychord_command_map_t* keychord_command_map = NULL;

int command_init() {

    command_t* command_cur;
    command_t* command_tmp;

    // Init Lua state
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);
    lua_settop(lua_state, 0);

    // Register commands
    COMMAND_REGISTER(commands, command_tmp, bind);
    COMMAND_REGISTER(commands, command_tmp, write);
    COMMAND_REGISTER(commands, command_tmp, write_newline);
    COMMAND_REGISTER(commands, command_tmp, write_tab);
    COMMAND_REGISTER(commands, command_tmp, delete_before);
    COMMAND_REGISTER(commands, command_tmp, delete_after);
    COMMAND_REGISTER(commands, command_tmp, cursor_move);
    COMMAND_REGISTER(commands, command_tmp, cursor_home);
    COMMAND_REGISTER(commands, command_tmp, cursor_end);
    COMMAND_REGISTER(commands, command_tmp, syntax_define);
    COMMAND_REGISTER(commands, command_tmp, syntax_add_rule);

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
    int new_line = 0;
    int new_offset = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    str = luaL_checkstring(lua_state, 1);

    buffer_insert_str(control->buffer, control->cursor_line, control->cursor_offset, str, 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_write_newline(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_insert_str(control->buffer, control->cursor_line, control->cursor_offset, "\n", 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_write_tab(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    int i;
    int num_spaces_to_tab_stop = 0;
    int tab_stop = 4; // TODO configurable
    char tab[tab_stop + 1];
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    num_spaces_to_tab_stop = tab_stop - (control->cursor_offset % tab_stop);
    strcpy(tab, "");
    for (i = 0; i < num_spaces_to_tab_stop; i++) {
        strcat(tab, " ");
    }

    buffer_insert_str(control->buffer, control->cursor_line, control->cursor_offset, tab, 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_delete_before(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    if (control->cursor_line < 1 && control->cursor_offset < 1) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_insert_str(control->buffer, control->cursor_line, control->cursor_offset - 1, "", 1, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_delete_after(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_insert_str(control->buffer, control->cursor_line, control->cursor_offset, "", 1, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_cursor_move(lua_State* lua_state) {

    int line = 0;
    int offset = 0;
    int line_delta = 0;
    int offset_delta = 0;
    int buffer_offset = 0;
    int target_line_offset = 0;
    int target_line_length = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    line = control->cursor_line;
    offset = control->cursor_offset;

    line_delta = luaL_checkint(lua_state, 1);
    offset_delta = luaL_checkint(lua_state, 2);

    line += line_delta;
    if (line < 0) {
        line = 0;
    } else if (line >= control->buffer->line_count){
        line = control->buffer->line_count - 1;
    }
    if (line_delta != 0) {
        buffer_get_line_offset_and_length(control->buffer, line, &target_line_offset, &target_line_length);
        if (control->cursor_offset_target < target_line_length) {
            target_line_length = control->cursor_offset_target;
        }
        offset = target_line_length;
    }
    offset += offset_delta;

    buffer_offset = buffer_get_buffer_offset(control->buffer, line, offset);
    buffer_get_line_and_offset(control->buffer, buffer_offset, &line, &offset);

    control_set_cursor(control, line, offset, line_delta == 0 ? TRUE : FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_cursor_home(lua_State* lua_state) {

    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    control_set_cursor(control, control->cursor_line, 0, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_cursor_end(lua_State* lua_state) {

    int offset = 0;
    int length = 0;
    control_t* control = control_get_active_buffer_view();

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }
    buffer_get_line_offset_and_length(control->buffer, control->cursor_line, &offset, &length);
    control_set_cursor(control, control->cursor_line, length, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_syntax_define(lua_State *L) {

    syntax_t* syntax;

    syntax = calloc(1, sizeof(syntax_t));
    syntax->name = strdup(luaL_checkstring(L, 1));
    syntax->regex_file_pattern = util_pcre_compile(luaL_checkstring(L, 2), NULL, NULL);
    HASH_ADD_KEYPTR(hh, syntaxes, syntax->name, strlen(syntax->name), syntax);

    lua_pushboolean(L, TRUE);
    return 1;

}

int command_execute_syntax_add_rule(lua_State *L) {

    syntax_t* syntax;
    syntax_rule_single_t** rule;
    char* name;
    char* regex;
    char* color_fg;
    char* color_bg;

    name = luaL_checkstring(L, 1);
    regex = luaL_checkstring(L, 2);
    color_fg = luaL_checkstring(L, 3);
    color_bg = luaL_checkstring(L, 4);

    HASH_FIND_STR(syntaxes, name, syntax);
    if (syntax == NULL) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    rule = &(syntax->rule_single_head);

    while (*rule != NULL) {
        rule = &((*rule)->next);
    }
    *rule = syntax_rule_single_new(regex, util_ncurses_getpair(color_fg, color_bg));
    (*rule)->regex_str = strdup(regex);

    lua_pushboolean(L, TRUE);
    return 1;

}
