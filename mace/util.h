#ifndef _UTIL_H
#define _UTIL_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pcre.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

char* util_lua_table_getstr(lua_State* L, char* key);
short util_ncurses_getcolorbystr(char* color);
int util_ncurses_getpair(char* fg_str, char* bg_str);
int util_ncurses_init_default_colors();
pcre* util_pcre_compile(char* regex, char* error, int* error_offset);
int util_file_exists(char* path);

#endif
