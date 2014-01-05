#include <ncurses.h>
#include <math.h>

#include "ext/uthash/uthash.h"

#include "control.h"
#include "buffer.h"
#include "highlighter.h"
#include "util.h"

extern FILE* fdebug;

control_t* title_bar;
control_t* multi_buffer_view;
control_t* status_bar;
control_t* prompt_bar;
control_t* active;
control_t* buffer_view_head;
control_t* buffer_view_tail;

int control_init() {

    ncurses_ensure_init();

    buffer_view_head = NULL;
    buffer_view_tail = NULL;

    title_bar = control_new();
    title_bar->window_attrs = A_REVERSE;

    status_bar = control_new();
    status_bar->window_attrs = A_REVERSE;

    prompt_bar = control_new_buffer_view();

    multi_buffer_view = control_new_multi_buffer_view();

    control_set_active_buffer_view(
        control_get_active_buffer_view()
    );

    control_resize();
    waddstr(title_bar->window, "  GNU mace 0.0.1");
    wclrtoeol(title_bar->window);

    return 0;
}

int ncurses_ensure_init() {
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    curs_set(1);
    return 0;
}

control_t* control_new() {
    control_t* control;
    control = (control_t*)calloc(1, sizeof(control_t));
    control->window = newwin(1, 1, 1, 1);
    control->resize = control_resize_default;
    control->render = control_render_default;
    control->window_line_num_width = 4; // TODO from config
    return control;
}

int control_resize_default(control_t* self, int width, int height, int left, int top) {
    self->width = width;
    self->height = height;
    self->left = left;
    self->top = top;
    wresize(self->window, height, width);
    mvwin(self->window, top, left);
    wbkgdset(self->window, self->window_attrs);
    return 0;
}

control_t* control_new_multi_buffer_view() {
    control_t* control = control_new();
    control->resize = control_resize_multi_buffer_view;
    control->render = control_render_multi_buffer_view;
    control->node_child = control_new_multi_buffer_view_node();
    control->node_active = control->node_child;
    return control;
}

control_t* control_new_multi_buffer_view_node() {
    control_t* control = control_new();
    control->resize = control_resize_multi_buffer_view_node;
    control->render = control_render_multi_buffer_view_node;
    control->buffer_view = control_new_buffer_view();
    return control;
}

control_t* control_new_buffer_view() {
    static int buffer_view_id = 1;
    control_t* control = control_new();
    control_t* tmp;
    control->window_line_num = newwin(1, 1, 1, 1);
    control->window_margin_left = newwin(1, 1, 1, 1);
    control->window_margin_right = newwin(1, 1, 1, 1);
    control->buffer = buffer_new();
    buffer_add_listener(control->buffer, control_on_dirty_lines, (void*)control);
    control->resize = control_resize_buffer_view;
    control->render = control_render_buffer_view;
    control->is_first_render = TRUE;
    control->buffer_view_id = buffer_view_id;
    buffer_view_id += 1;
    if (!buffer_view_head) {
        buffer_view_head = control;
        buffer_view_tail = control;
    } else {
        buffer_view_tail->next_buffer_view = control;
        buffer_view_tail = control;
    }
    return control;
}

int control_render_default(control_t* self) {
    wnoutrefresh(self->window);
    return 0;
}

int control_split_multi_buffer_view_node(control_t* orig, int split_type, int split_pos) {
    return 0;
}

int control_resize_multi_buffer_view(control_t* self, int width, int height, int left, int top) {
    control_t* cur = self->node_child;
    while (cur) {
        cur->resize(cur, width, height, left, top);
        cur = cur->node_sibling;
    }
    return 0;
}

int control_render_multi_buffer_view(control_t* self) {
    control_t* cur = self->node_child;
    while (cur) {
        cur->render(cur);
        cur = cur->node_sibling;
    }
    return 0;
}

int control_resize_multi_buffer_view_node(control_t* self, int width, int height, int left, int top) {
    self->width = width;
    self->height = height;
    self->left = left;
    self->top = top;
    if (self->is_split_container) {
        if (self->split_type == CONTROL_SPLIT_TYPE_HORIZONTAL) {
            self->split_pos = (int)floor(height * self->split_factor);
            self->split_node_orig->resize(self->split_node_orig, width, self->split_pos, left, top);
            self->split_node_new->resize(self->split_node_new, width, height - self->split_pos, left, top + self->split_pos);
        } else { // CONTROL_SPLIT_TYPE_VERTICAL
            self->split_pos = (int)floor(width * self->split_factor);
            self->split_node_orig->resize(self->split_node_orig, self->split_pos, height, left, top);
            self->split_node_new->resize(self->split_node_new, width - self->split_pos, height, left + self->split_pos, top);
        }
    } else {
        self->buffer_view->resize(self->buffer_view, width, height, left, top);
    }
    return 0;
}

int control_render_multi_buffer_view_node(control_t* self) {
    if (self->is_split_container) {
        self->split_node_orig->render(self->split_node_orig);
        self->split_node_new->render(self->split_node_new);
    } else {
        self->buffer_view->render(self->buffer_view);
    }
    return 0;
}

int control_resize_buffer_view(control_t* self, int width, int height, int left, int top) {
    self->width = width;
    self->height = height;
    self->left = left;
    self->top = top;

    wresize(self->window_line_num, height, self->window_line_num_width);
    mvwin(self->window_line_num, top, left);

    wresize(self->window_margin_left, height, 1);
    mvwin(self->window_margin_left, top, left + self->window_line_num_width);

    wresize(self->window_margin_right, height, 1);
    mvwin(self->window_margin_right, top, left + width - 1);

    wresize(self->window, height, width - self->window_line_num_width - 1 - 1);
    mvwin(self->window, top, left + self->window_line_num_width + 1);

    control_set_viewport(self, self->viewport_line_start, self->viewport_offset_start);

    return 0;
}

int control_render_buffer_view(control_t* self) {
    if (self->is_first_render) {
        buffer_dirty_lines(self->buffer, 0, self->buffer->line_count - 1, -1);
        //control_buffer_view_dirty_lines(self, 0, self->height - 1, FALSE);
        self->is_first_render = FALSE;
    }
    return 0;
}

int control_buffer_view_render_line(control_t* self, char* line, int line_on_screen, int line_in_buffer) {

    static char line_num_formatted[20]; // TODO #define
    static highlighted_substr_t* highlighted_substrs;
    static int highlighted_substr_count = 0;
    static int i;

    // TODO syntax highlighting
    if (self->highlighter != NULL) {
        highlighted_substrs = highlighter_highlight(self->highlighter, line, line_in_buffer, &highlighted_substr_count);
        wmove(self->window, line_on_screen, 0);
        for (i = 0; i < highlighted_substr_count; i++) {
            wattrset(self->window, highlighted_substrs[i].attrs);
            mvwaddnstr(self->window, line_on_screen, highlighted_substrs[i].start_offset, highlighted_substrs[i].substr, highlighted_substrs[i].length);
        }
    } else {
        mvwaddnstr(self->window, line_on_screen, 0, line, self->width);
    }
    wclrtoeol(self->window);

    if (line_in_buffer >= 0) {
        snprintf(line_num_formatted, self->window_line_num_width + 1, "% 4d", line_in_buffer + 1);
    } else {
        snprintf(line_num_formatted, self->window_line_num_width + 1, "% 4s", "~");
    }
    mvwaddnstr(self->window_line_num, line_on_screen, 0, line_num_formatted, self->window_line_num_width);

    // TODO actually use margin windows
    mvwaddnstr(self->window_margin_left, line_on_screen, 0, " ", 1);
    mvwaddnstr(self->window_margin_right, line_on_screen, 0, " ", 1);

    return 0;
}

int control_buffer_view_clear_line(control_t* self, int line_on_screen) {
    wmove(self->window, line_on_screen, self->viewport_offset_start);
    wclrtoeol(self->window);
    wmove(self->window_line_num, line_on_screen, 0);
    waddstr(self->window_line_num, "   ~"); // TODO not this
    return 0;
}

int control_buffer_view_clear_lines_from(control_t* self, int clear_line_start) {
    int i;
    if (self->viewport_line_start > clear_line_start) {
        clear_line_start = self->viewport_line_start;
    }
    for (i = clear_line_start; i <= self->viewport_line_end; i++) {
        control_buffer_view_clear_line(self, i);
    }
    return 0;
}

void control_on_dirty_lines(buffer_t* buffer, void* listener, int line_start, int line_end, int line_delta) {
    control_buffer_view_dirty_lines((control_t*)listener, line_start, line_end, line_delta);
}

int control_buffer_view_dirty_lines(control_t* self, int dirty_line_start, int dirty_line_end, int line_delta) {

    char** lines;
    char* line;
    int line_count;
    int i;

    if (self->viewport_line_start > dirty_line_start) {
        dirty_line_start = self->viewport_line_start;
    }
    if (self->viewport_line_end < dirty_line_end) {
        dirty_line_end = self->viewport_line_end;
    }

    lines = buffer_get_lines(self->buffer, dirty_line_start, dirty_line_end, &line_count);
    for (i = 0; i < line_count; i++) {
        line = *(lines + i);
        control_buffer_view_render_line(
            self,
            line ? line : "",
            dirty_line_start + i - self->viewport_line_start,
            line ? dirty_line_start + i : -1
        );
    }

    if (line_delta < 0) {
        control_buffer_view_clear_lines_from(self, dirty_line_end + 1);
    }

    wnoutrefresh(self->window);
    wnoutrefresh(self->window_line_num);
    wnoutrefresh(self->window_margin_left);
    wnoutrefresh(self->window_margin_right);

    return 0;
}

int control_render() {
    title_bar->render(title_bar);
    status_bar->render(status_bar);
    prompt_bar->render(prompt_bar);
    multi_buffer_view->render(multi_buffer_view);
    control_render_cursor();
    doupdate();
    return 0;
}

int control_render_cursor() {
    int top;
    int left;
    if (active != NULL) {
        getbegyx(active->window, top, left);
        move(top + active->cursor_line - active->viewport_line_start, left + active->cursor_offset - active->viewport_offset_start);
    }
    return 0;
}

int control_resize() {
    int width;
    int height;
    control_t* active_buffer_view;

    def_prog_mode();
    endwin();
    reset_prog_mode();
    refresh();

    getmaxyx(stdscr, height, width);

    title_bar->resize(title_bar, width, 1, 0, 0);
    status_bar->resize(status_bar, width, 1, 0, height - 2);
    prompt_bar->resize(prompt_bar, width, 1, 0, height - 1);
    multi_buffer_view->resize(multi_buffer_view, width, height - 3, 0, 1);

    active_buffer_view = control_get_active_buffer_view();
    if (active_buffer_view != NULL) {
        active_buffer_view->is_first_render = TRUE;
    }

    return 0;
}

control_t* control_get_active_buffer_view() {
    if (active && active->buffer) {
        return active;
    }
    return control_get_active_buffer_view_from_node(multi_buffer_view->node_active);
}

int control_set_active_buffer_view(control_t* bv) {
    active = bv;
    return 0;
}

control_t* control_get_active_buffer_view_from_node(control_t* node) {
    if (node->is_split_container) {
        return control_get_active_buffer_view_from_node(node->split_active);
    } else {
        return node->buffer_view;
    }
}

control_t* control_get_buffer_view_by_id(int id) {
    control_t* cur = buffer_view_head;
    if (id < 0) {
        return control_get_active_buffer_view();
    }
    while (cur) {
        if (cur->buffer_view_id == id) {
            return cur;
        }
        cur = cur->next_buffer_view;
    }
    return NULL;
}

int control_set_viewport(control_t* control, int line, int offset) {
    if (line < 0) {
        line = 0;
    }
    if (line >= control->buffer->line_count - control->height) {
        line = control->buffer->line_count - control->height - 1;
    }
    control->viewport_line_start = line;
    control->viewport_line_end = line + control->height - 1;
    control->viewport_offset_start = offset;
    control->viewport_offset_end = offset + control->width - 1;
    return 0;
}

int control_set_cursor(control_t* control, int line, int offset, bool set_target_offset) {

    int viewport_line_delta = 0;
    int viewport_offset_delta = 0;
    bool viewport_changed = FALSE;

    control->cursor_line = line;
    control->cursor_offset = offset;

    if (set_target_offset) {
        control->cursor_offset_target = offset;
    }
    if (line > control->viewport_line_end) {
        viewport_line_delta = line - control->viewport_line_end;
        control->viewport_line_start += viewport_line_delta;
        control->viewport_line_end += viewport_line_delta;
        viewport_changed = TRUE;
    } else if (line < control->viewport_line_start) {
        viewport_line_delta = control->viewport_line_start - line;
        control->viewport_line_start -= viewport_line_delta;
        control->viewport_line_end -= viewport_line_delta;
        viewport_changed = TRUE;
    }
    if (offset > control->viewport_offset_end) {
        viewport_offset_delta = offset - control->viewport_offset_end;
        control->viewport_offset_start += viewport_offset_delta;
        control->viewport_offset_end += viewport_offset_delta;
        viewport_changed = TRUE;
    } else if (offset < control->viewport_offset_start) {
        viewport_offset_delta = control->viewport_offset_start - offset;
        control->viewport_offset_start -= viewport_offset_delta;
        control->viewport_offset_end -= viewport_offset_delta;
        viewport_changed = TRUE;
    }

    if (viewport_changed) {
        control_buffer_view_dirty_lines(control, control->viewport_line_start, control->viewport_line_end, FALSE);
    }

    return 0;
}

int control_set_status(char* status) {
    mvwaddnstr(status_bar->window, 0, 0, status, status_bar->width);
    wclrtoeol(status_bar->window);
    return 0;
}
