#include "atto.h"

/**
 * Allocate a new bview
 */
bview_t* bview_new(buffer_t* opt_buffer, int is_chromeless) {
    bview_t* bview;
    bview = (bview_t*)calloc(1, sizeof(bview_t));
    bview->buffer = opt_buffer ? opt_buffer : buffer_new();
    bview->cursor = buffer_add_mark(bview->buffer, 0);
    bview->is_chromeless = is_chromeless;
    bview_viewport_set_scope(bview, 24); // TODO make configurable
    buffer_add_buffer_listener(bview->buffer, (void*)bview, _bview_buffer_callback);
    if (is_chromeless) {
        bview->win_buffer = newwin(1, 1, 0, 0);
    } else {
        bview->win_caption = newwin(1, 1, 0, 0);
        wbkgdset(bview->win_caption, A_REVERSE); // TODO window styles
        bview->win_lines = newwin(1, 1, 0, 0);
        bview->lines_width = 4; // TODO make configurable
        bview->win_margin_left = newwin(1, 1, 0, 0);
        bview->win_buffer = newwin(1, 1, 0, 0);
        bview->win_margin_right = newwin(1, 1, 0, 0);
    }
    keypad(bview->win_buffer, TRUE); // This makes wgetch return KEY_* constants
    return bview;
}

/**
 * Set viewport scope
 */
int bview_viewport_set_scope(bview_t* self, int viewport_scope) {
    if (viewport_scope == 0) {
        viewport_scope = 1;
    }
    self->viewport_scope = viewport_scope;
    return ATTO_RC_OK;
}

/**
 * Called when the underlying buffer is edited
 */
void _bview_buffer_callback(buffer_t* buffer, void* listener, int line, int col, char* delta, int delta_len) {
    bview_t* self;
    self = (bview_t*)listener;
    bview_update(self);
}

/**
 * Called when self->cursor is moved
 */
int _bview_update_viewport(bview_t* self, int line, int col) {
    _bview_update_viewport_dimension(self, line, self->viewport_h, &(self->viewport_y));
    _bview_update_viewport_dimension(self, col, self->viewport_w, &(self->viewport_x));
}

/**
 * Called by _bview_update_viewport for width and height
 */
int _bview_update_viewport_dimension(bview_t* self, int line, int viewport_h, int* viewport_y) {
    int scope_start;
    int scope_stop;
    int scope;

    if (self->viewport_scope < 0) {
        scope = (self->viewport_scope < (self->viewport_h * -1)) ? self->viewport_h * -1 : self->viewport_scope;
        scope_start = *viewport_y - scope;
        scope_stop = (*viewport_y + viewport_h) + scope;
    } else {
        scope = (self->viewport_scope > self->viewport_h) ? self->viewport_h : self->viewport_scope;
        scope_start = (*viewport_y + (viewport_h / 2)) - (int)floorf((float)scope * 0.5);
        scope_stop = (*viewport_y + (viewport_h / 2)) + (int)ceilf((float)scope * 0.5);
    }

    if (line < scope_start) {
        *viewport_y -= scope_start - line;
    } else if (line >= scope_stop) {
        *viewport_y += (line - scope_stop) + 1;
    }

    if (*viewport_y < 0) {
        *viewport_y = 0; // TODO make negative viewport an option
    }
    return ATTO_RC_OK;
}

/**
 * Update the bview
 */
int bview_update(bview_t* self) {
    int view_line;
    char* line_str;
    int line_num;
    char* line_num_str;
    int line_len;
    char margin_right;
    char margin_left;
    int is_viewport_x_on_cursor_only;
    int viewport_x;
    char line_format[] = "%4d";

    is_viewport_x_on_cursor_only = 1; // TODO make configurable
    line_str = (char*)calloc(self->viewport_w + 1, sizeof(char));
    line_num_str = (char*)calloc(self->lines_width + 1, sizeof(char));

    // Make line_format
    //line_format[1] = '0' + ATTO_MAX(ATTO_MIN(self->lines_width, 9), 0);

    // Render each line in viewport
    for (view_line = 0; view_line < self->viewport_h; view_line++) {
        line_num = self->viewport_y + view_line;
        viewport_x = is_viewport_x_on_cursor_only ? (line_num == self->cursor->line ? self->viewport_x : 0) : self->viewport_x;
        if (line_num < 0 || line_num >= self->buffer->line_count) {
            // No line exists here
            memset(line_str, ' ', self->viewport_w);
            memset(line_num_str, ' ', self->lines_width);
            line_num_str[self->lines_width - 1] = '~';
            margin_left = ' ';
            margin_right = ' ';
        } else {
            // TODO styles
            buffer_get_line(self->buffer, line_num, viewport_x, line_str, self->viewport_w, NULL, &line_len);
            memset(line_num_str, 'L', self->lines_width);
            snprintf(line_num_str, self->lines_width + 1, line_format, line_num + 1); // TODO 0-indexed lines option
            margin_left = (viewport_x > 0) ? '^' : ' ';
            margin_right = (self->viewport_w < line_len) ? '$' : ' ';
        }
        mvwaddnstr(self->win_lines, view_line, 0, line_num_str, self->lines_width);
        wclrtoeol(self->win_lines);
        mvwaddnstr(self->win_buffer, view_line, 0, line_str, self->viewport_w);
        wclrtoeol(self->win_buffer);
        mvwaddch(self->win_margin_left, view_line, 0, margin_left);
        wclrtoeol(self->win_margin_left);
        mvwaddch(self->win_margin_right, view_line, 0, margin_right);
        wclrtoeol(self->win_margin_right);
    }

    mvwaddnstr(self->win_caption, 0, 0, "[*] caption", self->width); // TODO caption

    wnoutrefresh(self->win_caption);
    wnoutrefresh(self->win_lines);
    wnoutrefresh(self->win_buffer);
    wnoutrefresh(self->win_margin_left);
    wnoutrefresh(self->win_margin_right);

    free(line_str);
    free(line_num_str);

    // Also update child
    if (self->split_child) {
        bview_update(self->split_child);
    }

    return ATTO_RC_OK;
}

/**
 * Show cursor
 */
int bview_update_cursor(bview_t* self) {
    wmove(self->win_buffer, self->cursor->line - self->viewport_y, self->cursor->col - self->viewport_x);
}

/**
 * Resize a bview
 */
int bview_resize(bview_t* self, int x, int y, int ow, int oh) {
    int w; // Width of split parent minus divider
    int h; // Height "
    int aw; // Width of split parent including divider
    int ah; // Height "

    // Default these values to the entire space
    w = ow;
    h = oh;
    aw = ow;
    ah = oh;

    // Adjust w,h,aw,ah to account for split_child
    if (self->split_child) {
        if (self->split_is_vertical) {
            aw = (int)((float)ow * self->split_factor);
            w = aw;
        } else {
            ah = (int)((float)oh * self->split_factor);
            h = ah;
        }
    }

    // Update dimensions
    self->left = x;
    self->top = y;
    self->width = aw;
    self->height = ah;

    if (self->is_chromeless) {
        // win_buffer takes up all the space in chromeless
        wresize(self->win_buffer, h, w);
        mvwin(self->win_buffer, y, x);
        wnoutrefresh(self->win_buffer);
        self->viewport_w = w;
        self->viewport_h = h;
    } else {
        // win_caption
        mvwin(self->win_caption, y, x);
        wresize(self->win_caption, 1, w);
        wnoutrefresh(self->win_caption);

        // win_lines
        mvwin(self->win_lines, y + 1, x);
        wresize(self->win_lines, h - 1, self->lines_width);
        wnoutrefresh(self->win_lines);

        // win_margin_left
        mvwin(self->win_margin_left, y + 1, x + self->lines_width);
        wresize(self->win_margin_left, h - 1, 1);
        wnoutrefresh(self->win_margin_left);

        // win_buffer
        self->viewport_w = w - (self->lines_width + 2);
        self->viewport_h = h - 1;
        mvwin(self->win_buffer, y + 1, x + self->lines_width + 1);
        wresize(self->win_buffer, self->viewport_h, self->viewport_w);
        wnoutrefresh(self->win_buffer);

        // win_margin_right
        mvwin(self->win_margin_right, y + 1, x + w - 1);
        wresize(self->win_margin_right, h - 1, 1);
        wnoutrefresh(self->win_margin_right);
    }

    if (self->split_child) {
        // Resize split_child
        bview_resize(
            self->split_child,
            self->split_is_vertical ? x + aw : x,
            self->split_is_vertical ? y : y + ah,
            self->split_is_vertical ? ow - aw : ow,
            self->split_is_vertical ? oh : oh - ah
        );
    }

    return ATTO_RC_OK;
}

/**
 * Move viewport by delta
 */
int bview_viewport_move(bview_t* self, int line_delta, int col_delta) {
    self->viewport_x += col_delta;
    self->viewport_y += line_delta;
    return ATTO_RC_OK;
}

/**
 * Set viewport
 */
int bview_viewport_set(bview_t* self, int line, int col) {
    self->viewport_x = col;
    self->viewport_y = line;
    return ATTO_RC_OK;
}

/**
 * Get parent in a split tree
 * Return self if self has no parent
 */
bview_t* bview_get_parent(bview_t* self) {
    if (self->split_parent) {
        return self->split_parent;
    }
    return self;
}

/**
 * Get top level parent in a split tree
 * Return self if self has no parent
 */
bview_t* bview_get_top_parent(bview_t* self) {
    bview_t* top_parent = self;
    while (top_parent->split_parent) {
        top_parent = top_parent->split_parent;
    }
    return top_parent;
}

/**
 * Split self into parent + child
 */
bview_t* bview_split(bview_t* self, int is_vertical, float factor) {
    bview_t* child;
    mark_t* cursor;
    mark_t* cursor_copy;
    keymap_node_t* keymapnode;
    keymap_node_t* keymapnode_copy;

    if (self->split_child) {
        // Already split
        return ATTO_RC_ERR;
    }

    // Make new child bview
    child = bview_new(self->buffer, self->is_chromeless);
    child->split_parent = self;

    // Set split details on parent
    self->split_child = child;
    self->split_factor = factor;
    self->split_is_vertical = is_vertical;

    // Copy viewport from parent
    child->viewport_x = self->viewport_x;
    child->viewport_y = self->viewport_y;

    // Copy cursors from parent
    mark_set(child->cursor, self->cursor->offset);
    child->cursor_list = NULL;
    LL_FOREACH(self->cursor_list, cursor) {
        cursor_copy = (mark_t*)malloc(sizeof(mark_t));
        memcpy(cursor_copy, cursor, sizeof(mark_t));
        LL_APPEND(child->cursor_list, cursor_copy);
    }

    // Copy keymaps, keymap_tail from parent
    child->keymaps = NULL;
    DL_FOREACH(self->keymaps, keymapnode) {
        keymapnode_copy = (keymap_node_t*)malloc(sizeof(keymap_node_t));
        memcpy(keymapnode_copy, keymapnode, sizeof(keymap_node_t));
        DL_APPEND(child->keymaps, keymapnode_copy);
        if (self->keymap_tail == keymapnode) {
            child->keymap_tail = keymapnode_copy;
        }
    }

    // Resize self
    bview_resize(self, self->left, self->top, self->width, self->height);

    // Update self
    bview_update(self);

    // Set active bview to child
    bview_set_active(child);

    return child;
}

/**
 * Push a keymap on bview
 */
int bview_keymap_push(bview_t* self, keymap_t* keymap) {
    keymap_node_t* node;
    node = (keymap_node_t*)calloc(1, sizeof(keymap_node_t));
    node->keymap = keymap;
    DL_APPEND(self->keymaps, node);
    self->keymap_tail = node;
    return ATTO_RC_OK;
}

/**
 * Pop a keymap
 */
keymap_t* bview_keymap_pop(bview_t* self) {
    keymap_t* popped;
    keymap_node_t* popped_node;
    popped = NULL;
    popped_node = NULL;
    if (self->keymap_tail) {
        popped_node = self->keymap_tail;
        DL_DELETE(self->keymaps, popped_node);
        popped = popped_node->keymap;
        self->keymap_tail = popped_node->prev;
        free(popped_node);
    }
    return popped;
}

/**
 * Set active bview
 */
int bview_set_active(bview_t* self) {
    g_bview_active = self;
    return ATTO_RC_OK;
}

/**
 * Get active bview
 */
bview_t* bview_get_active() {
    return g_bview_active;
}

/**
 * Destory a bview
 */
int bview_destroy(bview_t* self) {
    /*
    int left;
    int top;
    int width;
    int height;
    int is_chromeless;
    WINDOW* win_caption;
    WINDOW* win_lines;
    WINDOW* win_margin_left;
    WINDOW* win_buffer;
    WINDOW* win_margin_right;
    int lines_width;
    buffer_t* buffer;
    int viewport_x;
    int viewport_y;
    int viewport_w;
    int viewport_h;
    int viewport_scope;
    bview_t* split_child;
    float split_factor;
    int split_is_vertical;
    keymap_node_t* keymaps;
    keymap_node_t* keymap_tail;
    mark_t* cursor_list;
    mark_t* cursor;
    bview_t* next;
    bview_t* prev;
    */

    // TODO
    self = bview_get_top_parent(self);

    if (self->win_caption) delwin(self->win_caption);
    if (self->win_lines) delwin(self->win_lines);
    if (self->win_margin_left) delwin(self->win_margin_left);
    if (self->win_buffer) delwin(self->win_buffer);
    if (self->win_margin_right) delwin(self->win_margin_right);

    return ATTO_RC_OK;
}

int bview_set_prompt_label(char* label) {
    if (g_prompt_label_str) {
        free(g_prompt_label_str);
    }
    g_prompt_label_str = strdup(label);
    g_prompt_label_len = strlen(g_prompt_label_str);
    _layout_resize(g_width, g_height);
    if (g_prompt_label_len > 0) {
        mvwaddnstr(g_prompt_label, 0, 0, g_prompt_label_str, g_prompt_label_len);
    }
    wnoutrefresh(g_prompt_label);
    doupdate();
    return ATTO_RC_OK;
}

char* bview_get_prompt_label() {
    return g_prompt_label_str;
}
