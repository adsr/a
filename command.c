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
extern control_t* active;
extern control_t* prompt_bar;

/**
 * test multi
 * line highlighting
 * /* wee
 * include
 */ int test;

lua_State* lua_state;
command_t* commands = NULL;
int input_hook_ref = -1;

int command_init() {

    command_t* command_cur;
    command_t* command_tmp;

    // Init Lua state
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);
    lua_settop(lua_state, 0);

    // Register commands
    COMMAND_REGISTER(commands, command_tmp, buffer_write);
    COMMAND_REGISTER(commands, command_tmp, buffer_write_newline);
    COMMAND_REGISTER(commands, command_tmp, buffer_write_tab);
    COMMAND_REGISTER(commands, command_tmp, buffer_delete_before);
    COMMAND_REGISTER(commands, command_tmp, buffer_delete_after);
    COMMAND_REGISTER(commands, command_tmp, buffer_cursor_move);
    COMMAND_REGISTER(commands, command_tmp, buffer_cursor_home);
    COMMAND_REGISTER(commands, command_tmp, buffer_cursor_end);
    COMMAND_REGISTER(commands, command_tmp, syntax_define);
    COMMAND_REGISTER(commands, command_tmp, syntax_add_rule);
    COMMAND_REGISTER(commands, command_tmp, syntax_add_rule_multi);

    COMMAND_REGISTER(commands, command_tmp, input_set_hook);

    COMMAND_REGISTER(commands, command_tmp, buffer_get_prompt_id);
    COMMAND_REGISTER(commands, command_tmp, buffer_get_active_id);
    COMMAND_REGISTER(commands, command_tmp, buffer_set_active_id);
    COMMAND_REGISTER(commands, command_tmp, buffer_get_line);
    COMMAND_REGISTER(commands, command_tmp, buffer_clear);
    COMMAND_REGISTER(commands, command_tmp, buffer_find_all);
    COMMAND_REGISTER(commands, command_tmp, buffer_cursor_get);
    COMMAND_REGISTER(commands, command_tmp, buffer_cursor_set);
    COMMAND_REGISTER(commands, command_tmp, buffer_get_range);

    COMMAND_REGISTER(commands, command_tmp, highlight);
    COMMAND_REGISTER(commands, command_tmp, buffer_split);
    COMMAND_REGISTER(commands, command_tmp, buffer_unsplit);

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

int command_handle_keychord(keychord_t* keychord) {
    int error_code;
    if (input_hook_ref != -1) {
        lua_rawgeti(lua_state, LUA_REGISTRYINDEX, input_hook_ref);
    }
    //lua_getglobal(lua_state, "on_input"); // TODO constant
    lua_createtable(lua_state, 0, 2);
    lua_pushstring(lua_state, keychord->name);
    lua_setfield(lua_state, -2, "name");
    lua_pushstring(lua_state, keychord->ascii);
    lua_setfield(lua_state, -2, "ascii");
    error_code = lua_pcall(lua_state, 1, 0, 0);
    if (error_code != 0) {
        fprintf(fdebug, "got err from lua %d: %s\n", error_code, luaL_checkstring(lua_state, -1));
        lua_pop(lua_state, 1);
    }
    return 0;
}

int command_execute_buffer_write(lua_State* lua_state) {

    char* str;
    int new_line = 0;
    int new_offset = 0;
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    str = luaL_checkstring(lua_state, 2);

    buffer_splice(control->buffer, control->cursor_line, control->cursor_offset, str, 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_buffer_write_newline(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_splice(control->buffer, control->cursor_line, control->cursor_offset, "\n", 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_buffer_write_tab(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    int i;
    int num_spaces_to_tab_stop = 0;
    int tab_stop = 4; // TODO configurable
    char tab[tab_stop + 1];
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    num_spaces_to_tab_stop = tab_stop - (control->cursor_offset % tab_stop);
    strcpy(tab, "");
    for (i = 0; i < num_spaces_to_tab_stop; i++) {
        strcat(tab, " ");
    }

    buffer_splice(control->buffer, control->cursor_line, control->cursor_offset, tab, 0, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_buffer_delete_before(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    if (control->cursor_line < 1 && control->cursor_offset < 1) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_splice(control->buffer, control->cursor_line, control->cursor_offset - 1, "", 1, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_buffer_delete_after(lua_State* lua_state) {

    int new_line = 0;
    int new_offset = 0;
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_splice(control->buffer, control->cursor_line, control->cursor_offset, "", 1, &new_line, &new_offset);
    control_set_cursor(control, new_line, new_offset, FALSE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_buffer_cursor_move(lua_State* lua_state) {

    int line = 0;
    int offset = 0;
    int line_delta = 0;
    int offset_delta = 0;
    int buffer_offset = 0;
    int target_line_offset = 0;
    int target_line_length = 0;
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    line = control->cursor_line;
    offset = control->cursor_offset;

    line_delta = luaL_checkint(lua_state, 2);
    offset_delta = luaL_checkint(lua_state, 3);

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

int command_execute_buffer_cursor_home(lua_State* lua_state) {

    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    control_set_cursor(control, control->cursor_line, 0, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_buffer_cursor_end(lua_State* lua_state) {

    int offset = 0;
    int length = 0;
    int buffer_id = 0;
    control_t* control;

    buffer_id = luaL_checkint(lua_state, 1);
    control = control_get_buffer_view_by_id(buffer_id);

    if (!control || !control->buffer) {
        lua_pushboolean(lua_state, FALSE);
        return 1;
    }

    buffer_get_line_offset_and_length(control->buffer, control->cursor_line, &offset, &length);
    control_set_cursor(control, control->cursor_line, length, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;

}

int command_execute_buffer_get_prompt_id(lua_State* L) {
    lua_pushinteger(L, prompt_bar->buffer_view_id);
    return 1;
}

int command_execute_buffer_get_active_id(lua_State* L) {
    //control_t* active = control_get_active_buffer_view();
    lua_pushinteger(L, active->buffer_view_id);
    return 1;
}

int command_execute_buffer_set_active_id(lua_State* L) {

    control_t* buffer_view = command_get_buffer_view_by_id_at_arg(L, 1);
    if (!buffer_view) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    control_set_active_buffer_view(buffer_view);

}

control_t* command_get_buffer_view_by_id_at_arg(lua_State* L, int argn) {

    int buffer_id = 0;
    control_t* buffer_view;

    buffer_id = luaL_checkint(L, argn);
    buffer_view = control_get_buffer_view_by_id(buffer_id);

    if (!buffer_view || !buffer_view->buffer) {
        return NULL;
    }
    return buffer_view;

}

int command_execute_buffer_get_line(lua_State* L) {

    int line;
    char* str;
    control_t* buffer_view = command_get_buffer_view_by_id_at_arg(L, 1);
    if (!buffer_view) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    line = luaL_checkint(L, 2);

    str = buffer_get_line(buffer_view->buffer, line);
    if (str) {
        lua_pushstring(L, str);
    } else {
        lua_pushnil(L);
    }
    return 1;

}

int command_execute_buffer_clear(lua_State* L) {

    int new_line;
    int new_offset;
    control_t* buffer_view = command_get_buffer_view_by_id_at_arg(L, 1);
    if (!buffer_view) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    buffer_splice(buffer_view->buffer, 0, 0, "", buffer_view->buffer->char_count, &new_line, &new_offset);
    control_set_cursor(buffer_view, new_line, new_offset, TRUE);

    lua_pushboolean(lua_state, TRUE);
    return 1;
}

int command_execute_buffer_find_all(lua_State* L) {
}
int command_execute_buffer_cursor_get(lua_State* L) {
}
int command_execute_buffer_cursor_set(lua_State* L) {
}
int command_execute_buffer_get_range(lua_State* L) {
}
int command_execute_highlight(lua_State* L) {
}
int command_execute_buffer_split(lua_State* L) {
}
int command_execute_buffer_unsplit(lua_State* L) {
}

int command_execute_input_set_hook(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);
    input_hook_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

int command_execute_syntax_define(lua_State* L) {

    syntax_t* syntax;

    syntax = calloc(1, sizeof(syntax_t));
    syntax->name = strdup(luaL_checkstring(L, 1));
    syntax->regex_file_pattern = util_pcre_compile(luaL_checkstring(L, 2), NULL, NULL);
    HASH_ADD_KEYPTR(hh, syntaxes, syntax->name, strlen(syntax->name), syntax);

    lua_pushboolean(L, TRUE);
    return 1;

}

int command_execute_syntax_add_rule(lua_State* L) {

    syntax_t* syntax;
    syntax_rule_single_t** rule;
    char* name;
    char* regex;
    char* color_fg;
    char* color_bg;
    int other_attrs = 0;

    name = luaL_checkstring(L, 1);
    regex = luaL_checkstring(L, 2);
    color_fg = luaL_checkstring(L, 3);
    color_bg = luaL_checkstring(L, 4);
    other_attrs = luaL_checkint(L, 5);

    HASH_FIND_STR(syntaxes, name, syntax);
    if (syntax == NULL) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    rule = &(syntax->rule_single_head);

    while (*rule != NULL) {
        rule = &((*rule)->next);
    }
    *rule = syntax_rule_single_new(regex, util_ncurses_getpair(color_fg, color_bg) | other_attrs);
    (*rule)->regex_str = strdup(regex);

    lua_pushboolean(L, TRUE);
    return 1;

}

int command_execute_syntax_add_rule_multi(lua_State* L) {

    syntax_t* syntax;
    syntax_rule_multi_t** rule;
    char* name;
    char* regex_start;
    char* regex_end;
    char* color_fg;
    char* color_bg;
    int other_attrs = 0;

    name = luaL_checkstring(L, 1);
    regex_start = strdup(luaL_checkstring(L, 2));
    regex_end = strdup(luaL_checkstring(L, 3));
    color_fg = luaL_checkstring(L, 4);
    color_bg = luaL_checkstring(L, 5);
    other_attrs = luaL_checkint(L, 6);

    HASH_FIND_STR(syntaxes, name, syntax);
    if (syntax == NULL) {
        lua_pushboolean(L, FALSE);
        return 1;
    }

    rule = &(syntax->rule_multi_head);

    while (*rule != NULL) {
        rule = &((*rule)->next);
    }
    *rule = syntax_rule_multi_new(regex_start, regex_end, util_ncurses_getpair(color_fg, color_bg) | other_attrs);
    (*rule)->regex_start_str = regex_start;
    (*rule)->regex_end_str = regex_end;

    lua_pushboolean(L, TRUE);
    return 1;

}
