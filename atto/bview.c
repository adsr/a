#include "atto.h"

static void bview_init(bview_t* self, buffer_t* buffer);
static void bview_deinit(bview_t* self);
static void bview_handle_buffer_action(void* listener, buffer_t* buffer, baction_t* baction);

/** Create a new bview */
int bview_new(char* opt_filename, int opt_filename_len, bview_t** ret_bview) {
    bview_t* self;
    buffer_t* buffer;
    int rc;

    // Open buffer
    if (opt_filename) {
        if ((rc = buffer_open(opt_filename, opt_filename_len, &buffer)) != ATTO_RC_OK) {
            return rc;
        }
    } else {
        buffer_new(&buffer);
    }

    // Allocate
    self = calloc(1, sizeof(bview_t));
    self->rect_caption.bg = TB_REVERSE;
    self->rect_lines.fg = TB_YELLOW;
    self->rect_margin_left.fg = TB_RED;
    self->rect_margin_right.fg = TB_RED;
    bview_init(self, buffer);

    *ret_bview = self;
    ATTO_RETURN_OK;
}

/** Move and resize a bview to the given position and dimensions */
int bview_resize(bview_t* self, int x, int y, int w, int h) {
    int aw, ah;

    aw = w;
    ah = h;

    if (self->split_child) {
        if (self->split_is_vertical) {
            aw = ATTO_MAX(1, (int)((float)aw * self->split_factor));
        } else {
            ah = ATTO_MAX(1, (int)((float)ah * self->split_factor));
        }
    }

    self->x = x;
    self->y = y;
    self->w = aw;
    self->h = ah;

    if (self->is_chromeless) {
        self->rect_buffer.x = x;
        self->rect_buffer.y = y;
        self->rect_buffer.w = aw;
        self->rect_buffer.h = ah;
    } else {
        self->rect_caption.x = x;
        self->rect_caption.y = y;
        self->rect_caption.w = aw;
        self->rect_caption.h = 1;

        self->rect_lines.x = x;
        self->rect_lines.y = y + 1;
        self->rect_lines.w = self->line_num_width;
        self->rect_lines.h = ah - 1;

        self->rect_margin_left.x = x + self->line_num_width;
        self->rect_margin_left.y = y + 1;
        self->rect_margin_left.w = 1;
        self->rect_margin_left.h = ah - 1;

        self->rect_buffer.x = x + self->line_num_width + 1;
        self->rect_buffer.y = y + 1;
        self->rect_buffer.w = aw - (self->line_num_width + 1 + 1);
        self->rect_buffer.h = ah - 1;

        self->rect_margin_right.x = x + (aw - 1);
        self->rect_margin_right.y = y + 1;
        self->rect_margin_right.w = 1;
        self->rect_margin_right.h = ah - 1;
    }

    if (self->split_child) {
        bview_resize(self->split_child, x + aw, y + ah, w - aw, h - ah);
    }

    ATTO_RETURN_OK;
}

/** Open a new file in this bview */
int bview_open(bview_t* self, char* filename, int filename_len) {
    buffer_t* buffer;
    int rc;
    // TODO hook, chance to cancel, save unsaved changes etc
    if ((rc = buffer_open(filename, filename_len, &buffer)) != ATTO_RC_OK) {
        return rc;
    }
    bview_deinit(self);
    buffer_destroy(self->buffer);
    bview_init(self, buffer);
    ATTO_RETURN_OK;
}

/** Draw bview to screen */
int bview_draw(bview_t* self) {
    int screen_line;
    ssize_t line_index;
    buffer_t* buffer;
    bline_t* bline;
    int is_active_line;
    uint32_t uc;
    char* line_data;
    size_t char_offset;
    size_t width_consumed;
    int screen_offset;

    // Draw caption
    if (!self->is_chromeless) {
        tb_printf(
            self->rect_caption,
            0, 0, 0, 0 | (self == editor->active ? TB_BOLD : 0),
            "[%c] %-*.*s",
            self->buffer->has_unsaved_changes ? '*' : ' ',
            self->rect_caption.w - 4, self->rect_caption.w - 4, self->buffer->filename
        );

    }

    // Draw buffer
    buffer = self->buffer;
    for (screen_line = 0; screen_line < self->rect_buffer.h; screen_line++) {

        // Get bline at this screen line
        line_index = self->viewport_y + screen_line;
        if (line_index >= 0 && line_index < buffer->line_count) {
            buffer_get_bline(buffer, line_index, &bline);
        } else {
            bline = NULL;
        }
        is_active_line = (bline && self->active_cursor->mark->bline == bline ? 1 : 0);

        // Line numbers
        if (!self->is_chromeless && self->line_num_width > 0) {
            if (bline) {
                tb_printf(
                    self->rect_lines,
                    0, screen_line, 0, 0,
                    "%*d",
                    self->line_num_width, bline->line_index + 1
                );
            } else {
                tb_printf(
                    self->rect_lines,
                    0, screen_line, 0, 0,
                    "%*.*s",
                    self->line_num_width, self->line_num_width, ""
                );
            }
        }

        // Margins
        if (!self->is_chromeless) {
            // Left margin
            tb_printf(
                self->rect_margin_left,
                0, screen_line, 0, 0,
                "%c",
                (self->viewport_x > 0 && bline && is_active_line ? '^' : ' ')
            );
            // Right margin
            tb_printf(
                self->rect_margin_right,
                0, screen_line, 0, 0,
                "%c",
                (bline && bline->width > self->rect_buffer.w && bline->width - self->viewport_x > self->rect_buffer.w ? '$' : ' ')
            );
        }

        // Line data
        if (bline) {
            line_data = bline->data;
            char_offset = 0;
            if (self->viewport_x > 0 && is_active_line) {
                // Consume some chars if the horizontal viewport is shifted
                for (width_consumed = 0; char_offset < bline->char_count && line_data && self->viewport_x > width_consumed; char_offset++) {
                    line_data += tb_utf8_char_to_unicode(&uc, line_data);
                    width_consumed += bline->char_widths[char_offset];
                }
            }
            for (screen_offset = 0; screen_offset < self->rect_buffer.w; char_offset++) {
                if (char_offset < bline->char_count && line_data) {
                    // Print styled character
                    line_data += tb_utf8_char_to_unicode(&uc, line_data);
                    //tb_change_cell(self->rect_buffer.x + screen_offset, self->rect_buffer.y + screen_line, uc, ATTO_SBLOCK_FG(bline->char_styles[char_offset]), ATTO_SBLOCK_BG(bline->char_styles[char_offset]));
                    tb_change_cell(self->rect_buffer.x + screen_offset, self->rect_buffer.y + screen_line, uc, 0, 0);
                    screen_offset += bline->char_widths[char_offset];
                } else {
                    // Print blank space til end of line
                    tb_change_cell(self->rect_buffer.x + screen_offset, self->rect_buffer.y + screen_line, (uint32_t)' ', TB_DEFAULT, TB_DEFAULT);
                    screen_offset += 1;
                }
            }
        } else {
            // No line here, so print blank spaces
            for (screen_offset = 0; screen_offset < self->rect_buffer.w; screen_offset++) {
                tb_change_cell(self->rect_buffer.x + screen_offset, self->rect_buffer.y + screen_line, (uint32_t)' ', TB_DEFAULT, TB_DEFAULT);
            }
        }
    }

    // Draw child if it exists
    if (self->split_child) {
        bview_draw(self->split_child);
    }

    ATTO_RETURN_OK;
}

/** TODO bview_get_absolute_cursor_coords */
int bview_get_absolute_cursor_coords(bview_t* self, int *ret_cx, int *ret_cy) {
    // TODO maybe change all size_t to ssize_t for consistency?
    *ret_cx = self->rect_buffer.x + ((ssize_t)self->active_cursor->mark->char_offset - self->viewport_x);
    *ret_cy = self->rect_buffer.y + ((ssize_t)self->active_cursor->mark->bline->line_index - self->viewport_y);
    ATTO_RETURN_OK;
}

/** TODO bview_move_viewport */
int bview_move_viewport(bview_t* self, int line_delta, int col_delta) {
    ATTO_RETURN_OK;
}

/** TODO bview_set_viewport */
int bview_set_viewport(bview_t* self, bline_t* bline, size_t col) {
    ATTO_RETURN_OK;
}

/** Set bview viewport scope. */
int bview_set_viewport_scope(bview_t* self, ssize_t viewport_scope_x, ssize_t viewport_scope_y) {
    // If viewport_scope_* is negative, we try to maintain
    // n=abs(viewport_scope_*) lines/chars of padding between the cursor and
    // the edges of the screen. If viewport_scope_* is positive, we try to
    // maintain n=(viewport_h - viewport_scope_*) lines/chars of padding.
    self->viewport_scope_x = viewport_scope_x;
    self->viewport_scope_y = viewport_scope_y;
    ATTO_RETURN_OK;
}

/** TODO bview_split */
int bview_split(bview_t* self, int is_vertical, float factor, bview_t** ret_bview) {
    ATTO_RETURN_OK;
}

/** TODO bview_get_parent */
int bview_get_parent(bview_t* self, bview_t** ret_bview) {
    ATTO_RETURN_OK;
}

/** TODO bview_get_top_parent */
int bview_get_top_parent(bview_t* self, bview_t** ret_bview) {
    ATTO_RETURN_OK;
}

/** Push a keymap on the stack */
int bview_push_keymap(bview_t* self, keymap_t* keymap) {
    keymap_node_t* node;
    node = calloc(1, sizeof(keymap_node_t));
    node->keymap = keymap;
    DL_APPEND(self->keymap_nodes, node);
    self->keymap_node_tail = node;
    ATTO_RETURN_OK;
}

/** Pop a keymap off the stack */
int bview_pop_keymap(bview_t* self, keymap_t** ret_keymap) {
    keymap_node_t* newtail;
    if (!self->keymap_node_tail) {
        ATTO_RETURN_ERR("%s", "No keymaps on the stack");
    }
    newtail = self->keymap_node_tail->prev;
    DL_DELETE(self->keymap_nodes, self->keymap_node_tail);
    free(self->keymap_node_tail);
    self->keymap_node_tail = newtail;
    ATTO_RETURN_OK;
}

/** Add a cursor */
int bview_add_cursor(bview_t* self, bline_t* bline, size_t offset, int set_active, int is_asleep, cursor_t** ret_cursor) {
    cursor_new(self, bline, offset, is_asleep, ret_cursor);
    DL_APPEND(self->cursors, *ret_cursor);
    if (set_active || !self->active_cursor) {
        bview_set_active_cursor(self, *ret_cursor);
    }
    ATTO_RETURN_OK;
}

/** TODO bview_remove_cursor */
int bview_remove_cursor(bview_t* self, cursor_t* cursor) {
    cursor_t* target;
    cursor_t* tmp;
    DL_FOREACH_SAFE(self->cursors, target, tmp) {
        if (target == cursor) {
            if (self->active_cursor == cursor) {
                if (cursor->prev) {
                    bview_set_active_cursor(self, cursor->prev);
                } else if (cursor->next) {
                    bview_set_active_cursor(self, cursor->next);
                }
            }
            DL_DELETE(self->cursors, cursor);
            cursor_destroy(cursor);
            ATTO_RETURN_OK;
        }
    }
    ATTO_RETURN_ERR("Cursor %p not found", cursor);
}

/** TODO bview_get_active_cursor */
int bview_get_active_cursor(bview_t* self, cursor_t** ret_cursor) {
    *ret_cursor = self->active_cursor;
    ATTO_RETURN_OK;
}

/** TODO bview_set_active_cursor */
int bview_set_active_cursor(bview_t* self, cursor_t* cursor) {
    cursor_t* target;
    DL_FOREACH(self->cursors, target) {
        if (target == cursor) {
            self->active_cursor = cursor;
            ATTO_RETURN_OK;
        }
    }
    ATTO_RETURN_ERR("Cursor %p not found", cursor);
}

/** TODO bview_next_active_cursor */
int bview_next_active_cursor(bview_t* self, cursor_t** ret_cursor) {
    ATTO_RETURN_OK;
}

/** TODO bview_prev_active_cursor */
int bview_prev_active_cursor(bview_t* self, cursor_t** ret_cursor) {
    ATTO_RETURN_OK;
}

/** TODO bview_collapse_active_cursor */
int bview_collapse_active_cursor(bview_t* self) {
    ATTO_RETURN_OK;
}

/** TODO bview_destroy */
int bview_destroy(bview_t* self) {
    ATTO_RETURN_OK;
}

// ============================================================================

/** Init a bview with a buffer */
static void bview_init(bview_t* self, buffer_t* buffer) {
    cursor_t* cursor_tmp;

    self->buffer = buffer;

    bview_deinit(self);

    self->line_num_width = self->buffer->line_digits;

    bview_set_viewport(self, self->buffer->first_line, 0);
    bview_set_viewport_scope(self, -20, 1); // TODO configurable/formulaic default viewport scope

    bview_push_keymap(self, editor->default_keymap);

    bview_add_cursor(self, self->buffer->first_line, 0, 1, 0, &cursor_tmp);

    buffer_add_buffer_listener(buffer, (void*)self, bview_handle_buffer_action, &(self->blistener));
}

/** Deinit a bview */
static void bview_deinit(bview_t* self) {
    keymap_t* keymap_tmp;
    while (self->keymap_nodes) {
        bview_pop_keymap(self, &keymap_tmp);
    }
    while (self->active_cursor) {
        bview_remove_cursor(self, self->active_cursor);
    }
    if (self->buffer && self->blistener) {
        buffer_remove_buffer_listener(self->buffer, self->blistener);
    }
}

/** Invoked when an edit takes place on a buffer */
static void bview_handle_buffer_action(void* listener, buffer_t* buffer, baction_t* baction) {
    bview_t* self;
    self = (bview_t*)listener;

    if (baction->delta.line_delta != 0) {
        self->line_num_width = buffer->line_digits;
    }
}
