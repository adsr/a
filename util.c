#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <ncurses.h>
#include <pcre.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "util.h"

extern FILE* fdebug;

short ncurses_default_color_bg = -1;
short ncurses_default_color_fg = -1;

char* util_lua_table_getstr(lua_State* L, char* key) {
    // Assumes table is at stac k index -1
    size_t size_tmp;
    char* str;
    char* retval;
    lua_pushstring(L, key);
    lua_gettable(L, -2); // Table now at -2
    str = lua_tolstring(L, -1, &size_tmp);
    if (size_tmp < 1) {
        retval = NULL;
    } else {
        retval = (char*)malloc(size_tmp);
        strcpy(retval, str);
    }
    return retval;
}

short util_ncurses_getcolorbystr(char* color) {
    if (ncurses_default_color_bg == -1) {
        util_ncurses_init_default_colors();
    }
    if (strcmp(color, "black") == 0) {
        return COLOR_BLACK;
    } else if (strcmp(color, "red") == 0) {
        return COLOR_RED;
    } else if (strcmp(color, "green") == 0) {
        return COLOR_GREEN;
    } else if (strcmp(color, "yellow") == 0) {
        return COLOR_YELLOW;
    } else if (strcmp(color, "blue") == 0) {
        return COLOR_BLUE;
    } else if (strcmp(color, "magenta") == 0) {
        return COLOR_MAGENTA;
    } else if (strcmp(color, "cyan") == 0) {
        return COLOR_CYAN;
    } else if (strcmp(color, "white") == 0) {
        return COLOR_WHITE;
    } else if (strcmp(color, "default_bg") == 0) {
        return ncurses_default_color_bg;
    } else if (strcmp(color, "default_fg") == 0) {
        return ncurses_default_color_fg;
    }
    return 0;
}

int util_ncurses_getpair(char* fg_str, char* bg_str) {

    static short pair_count = 0;
    short pair_start = 2;
    short pair_end = pair_start + pair_count - 1;
    short pair;
    short fg_num = util_ncurses_getcolorbystr(fg_str);
    short bg_num = util_ncurses_getcolorbystr(bg_str);
    short tmp_fg_num;
    short tmp_bg_num;

    for (pair = pair_start; pair <= pair_end; pair++) {
        pair_content(pair, &tmp_fg_num, &tmp_bg_num);
        if (tmp_fg_num == fg_num && tmp_bg_num == bg_num) {
            return COLOR_PAIR(pair);
        }
    }

    pair = pair_start + pair_count;
    if (pair >= COLOR_PAIRS) {
        // Too many pairs defined
        return 0;
    }

    init_pair(pair, fg_num, bg_num);
    pair_count += 1;
    return COLOR_PAIR(pair);
}

int util_ncurses_init_default_colors() {
    return pair_content(0, &ncurses_default_color_fg, &ncurses_default_color_bg);
}

pcre* util_pcre_compile(char* regex, char* error, int* error_offset) {
    pcre* re;
    if (error == NULL) {
        error = (char*)malloc(sizeof(char));
    }
    if (error_offset == NULL) {
        error_offset = (int*)malloc(sizeof(int));
    }
    re = pcre_compile(
        regex,
        PCRE_NO_AUTO_CAPTURE,
        &error,
        error_offset,
        NULL
    );
    return re;
}

int util_file_exists(char* path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}
