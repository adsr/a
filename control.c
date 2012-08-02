#include <ncurses.h>
#include <math.h>

#include "control.h"
#include "buffer.h"

extern FILE* fdebug;

control_t* title_bar;
control_t* multi_buffer_view;
control_t* status_bar;
control_t* prompt_bar;
control_t* active;

int control_init() {

    // Init ncurses
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    //init_pair(1, COLOR_BLACK, COLOR_RED);
    curs_set(0);

    title_bar = control_new();
    title_bar->window_attrs = A_REVERSE;

    status_bar = control_new();
    status_bar->window_attrs = A_REVERSE;

    prompt_bar = control_new();

    multi_buffer_view = control_new_multi_buffer_view();

    active = control_get_active_buffer_view();

    control_resize();
    waddstr(title_bar->window, "  GNU mace 0.0.1");
    wclrtoeol(title_bar->window);

    return 0;
}

control_t* control_new() {
    control_t* control;
    control = (control_t*)calloc(1, sizeof(control_t));
    control->window = newwin(1, 1, 1, 1);
    control->resize = control_resize_default;
    control->render = control_render_default;
    control->window_line_num_width = 4;
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
    control_t* control = control_new();
    control->buffer = buffer_new();
    buffer_add_listener(control->buffer, control);
    control->resize = control_resize_buffer_view;
    control->render = control_render_buffer_view;
    control->is_first_render = TRUE;
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
    mvwin(self->window_margin_left, top, left + 1);

    wresize(self->window_margin_right, height, 1);
    mvwin(self->window_margin_right, top, left + width - 1);

    wresize(self->window, height, width - self->window_line_num_width - 1 - 1);
    mvwin(self->window, top, left + self->window_line_num_width + 1);

    self->viewport_line_end = self->viewport_line_start + height - 1;

    return 0;
}

int control_render_buffer_view(control_t* self) {
    if (self->is_first_render) {
        control_buffer_view_dirty_lines(self, 0, self->height - 1, FALSE);
        self->is_first_render = FALSE;
    }
    return 0;
}

int control_buffer_view_render_line(control_t* self, char* line, int line_on_screen, int line_in_buffer) {
    //char* line_formatted = "    ";
    // TODO syntax highlighting

    //fprintf(stderr, "printing [%s] screen_line=%d real_line=%d\n", line, line_on_screen, line_in_buffer);
    wmove(self->window, line_on_screen, self->viewport_offset);
    waddstr(self->window, line);
    wclrtoeol(self->window);
    wmove(self->window_line_num, line_on_screen, 0);

    //snprintf(line_formatted, 4, "% 4d", line_in_buffer + 1);
    //waddstr(self->window_line_num, line_formatted);
    return 0;
}

int control_buffer_view_clear_line(control_t* self, int line_on_screen) {
    wmove(self->window, line_on_screen, self->viewport_offset);
    wclrtoeol(self->window);
    wmove(self->window_line_num, line_on_screen, 0);
    waddstr(self->window_line_num, "   ~");
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

int control_buffer_view_dirty_lines(control_t* self, int dirty_line_start, int dirty_line_end, bool line_count_decreased) {

    char** lines;
    int line_count;
    int i;

    if (dirty_line_start > self->viewport_line_end
        || dirty_line_end < self->viewport_line_start
    ) {
        return 0;
    }

    if (self->viewport_line_start > dirty_line_start) {
        dirty_line_start = self->viewport_line_start;
    }

    if (self->viewport_line_end < dirty_line_end) {
        dirty_line_end = self->viewport_line_end;
    }

    lines = buffer_get_lines(self->buffer, dirty_line_start, dirty_line_end, &line_count);
    for (i = 0; i < line_count; i++) {
        control_buffer_view_render_line(self, *(lines + i), dirty_line_start + i - self->viewport_line_start, dirty_line_start + i);
    }

    if (line_count_decreased) {
        control_buffer_view_clear_lines_from(self, dirty_line_end + 1);
    }

    wnoutrefresh(self->window);
    wnoutrefresh(self->window_line_num);
    //wnoutrefresh(self->window_margin_left);
    //wnoutrefresh(self->window_margin_right);

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
    int cursor_line;
    int cursor_offset;
    if (active != NULL) {
        control_get_cursor(active, &cursor_line, &cursor_offset);
        curs_set(2);
        wmove(active->window, cursor_line, cursor_offset);
    }
    return 0;
}

int control_resize() {
    int width;
    int height;

    getmaxyx(stdscr, height, width);

    title_bar->resize(title_bar, width, 1, 0, 0);
    status_bar->resize(status_bar, width, 1, 0, height - 2);
    prompt_bar->resize(prompt_bar, width, 1, 0, height - 1);
    multi_buffer_view->resize(multi_buffer_view, width, height - 3, 0, 1);

    endwin();
    refresh();

    return 0;
}

control_t* control_get_active() {
    return active;
}

control_t* control_get_active_buffer_view() {
    return control_get_active_buffer_view_from_node(multi_buffer_view->node_active);
}

control_t* control_get_active_buffer_view_from_node(control_t* node) {
    if (node->is_split_container) {
        return control_get_active_buffer_view_from_node(node->split_active);
    } else {
        return node->buffer_view;
    }
}

int control_set_cursor(control_t* control, int line, int offset) {
    control->cursor_line = line;
    control->cursor_offset = offset;
    control->cursor_offset_target = offset;
    return 0;
}

int control_get_cursor(control_t* control, int* line, int* offset) {
    *line = control->cursor_line;
    *offset = control->cursor_offset;
    return 0;
}

int control_set_status(char* status) {
    mvwaddnstr(status_bar->window, 0, 0, status, status_bar->width);
    wclrtoeol(status_bar->window);
    return 0;
}
