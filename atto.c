#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <wctype.h>
#include <locale.h>
#include <langinfo.h>
#include <pcre.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncursesw/ncurses.h>
#include "ext/bstrlib/bstrlib.h"
#include "ext/bstrlib/bstraux.h"
#include "ext/uthash/uthash.h"
#include "ext/uthash/utarray.h"
#include "ext/uthash/utlist.h"

typedef struct buffer_s buffer_t;
typedef struct buffer_view_s buffer_view_t;
typedef struct buffer_listener_s buffer_listener_t;
typedef struct syntax_s syntax_t;
typedef struct highlighter_s highlighter_t;
typedef struct highlighter_rule_s highlighter_rule_t;
typedef struct highlighter_rule_workspace_s highlighter_rule_workspace_t;

struct buffer_s {
    bstring content;
    size_t byte_count;
    size_t char_count;
    size_t line_count;
    UT_array* line_offsets;
    buffer_listener_t* buffer_listener_head;
    int buffer_id;
    char* filename;
    syntax_t* syntax;
    int is_dirty;
    int is_read_only;
};

struct buffer_view_s {
    buffer_t* buffer;
    int width;
    int height;
    int left;
    int top;
    int cursor_line;
    int cursor_offset;
    int cursor_offset_target;
    int viewport_line_start;
    int viewport_height;
    int viewport_offset_start;
    int viewport_width;
    int split_type;
    float split_ratio;
    int split_pos;
    buffer_view_t* split_child;
    WINDOW* window_split;
    WINDOW* window_title;
    WINDOW* window_buffer;
    WINDOW* window_line_num;
    WINDOW* window_margin_left;
    WINDOW* window_margin_right;
};

typedef void (*buffer_listener_fn)(
    buffer_t* buffer,
    void* listener,
    int line,
    int column,
    int char_offset,
    int byte_offset,
    int char_delta,
    int byte_delta,
    int line_delta,
    char* deleted,
    char* inserted
);

struct buffer_listener_s {
    void* listener;
    buffer_listener_fn fn;
    void* next;
};

/*
single, multi, range, adhoc

adhoc: multi-line regex
range: start char, end char
single: single-line regex
multi: start regex, end regex

*/

int main(int argc, char** argv) {

    char* x = setlocale(LC_ALL, "");
    x = setlocale(LC_ALL, NULL);
    int utf8_mode = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0);

    printf("locale=%s utf_mode=%d\n", x, (int)utf8_mode);

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    //curs_set(1);

    wint_t wc = L'\0';

    cchar_t cc;
    get_wch(&wc);
    setcchar(&cc, &wc, 0, 0, NULL);
    add_wch(&cc);

    refresh();

    getch();

    clear();
    endwin();

    char c[MB_LEN_MAX];
    int len = wctomb(c, wc);
    printf("wc=%d len=%d\n", wc, len);

    return EXIT_SUCCESS;
}
