#include "atto.h"

static void bline_add_mark(bline_t* bline, mark_t* mark);
static int mark_set_pos_ex(mark_t* self, bline_t* bline, size_t char_offset, int set_target_char_offset);
static void bview_update_viewport(bview_t* bview, mark_t* self);
static void bview_update_viewport_dimension(bview_t* self, size_t cursor_coord, ssize_t viewport_scope, ssize_t viewport_h, ssize_t viewport_y, ssize_t* ret_viewport_y);

/** Create a new mark */
int mark_new(bline_t* bline, size_t offset, char* name, int name_len, cursor_t* opt_parent, mark_t** ret_mark) {
    mark_t* mark;
    size_t boffset;

    buffer_sanitize_position(bline->buffer, bline, offset, 0, &bline, &offset, &boffset);

    // Create mark
    mark = calloc(1, sizeof(mark_t));
    mark->bline = bline;
    mark->char_offset = offset;
    mark->target_char_offset = offset;
    mark->boffset = boffset;
    mark->cursor = opt_parent;
    snprintf(mark->name, ATTO_MARK_MAX_NAME_LEN + 1, "%.*s", name_len, name); // TODO optional or nix

    // Register with bline and buffer
    bline_add_mark(bline, mark);

    *ret_mark = mark;
    ATTO_RETURN_OK;
}

/** TODO mark_get_char_offset */
int mark_get_char_offset(mark_t* self, size_t* ret_offset) {
}

/** TODO mark_get_byte_offset */
int mark_get_byte_offset(mark_t* self, size_t* ret_offset) {
}

/** TODO mark_get_char_after */
int mark_get_char_after(mark_t* self, uint32_t* ret_char) {
}

/** TODO mark_get_char_before */
int mark_get_char_before(mark_t* self, uint32_t* ret_char) {
}

/** Move mark by char_delta characters */
int mark_move(mark_t* self, ssize_t char_delta) {
    bline_t* bline;
    size_t char_offset;
    buffer_sanitize_position(self->bline->buffer, NULL, 0, ATTO_MAX(0, (ssize_t)self->boffset + char_delta), &bline, &char_offset, NULL);
    return mark_set_pos(self, bline, char_offset);
}

/** TODO mark_move_vert */
int mark_move_vert(mark_t* self, ssize_t num_lines) {
    int incr;
    bline_t* target_bline;
    bline_t* tmp_bline;
    size_t target_index;
    incr = (num_lines > 0 ? -1 : 1);
    target_bline = self->bline;
    target_index = ATTO_MIN(target_bline->buffer->line_count - 1, ATTO_MAX(0, (ssize_t)target_bline->line_index + num_lines));
    while (num_lines != 0 && target_bline->line_index != target_index) {
        tmp_bline = (num_lines > 0 ? target_bline->next : target_bline->prev);
        if (!tmp_bline) {
            break;
        }
        target_bline = tmp_bline;
        num_lines += incr;
    }
    return mark_set_pos_ex(self, target_bline, ATTO_MIN(target_bline->char_count, self->target_char_offset), 0);
}

/** TODO mark_move_bol */
int mark_move_bol(mark_t* self) {
    return mark_set_pos_ex(self, self->bline, 0, 1);
}

/** TODO mark_move_eol */
int mark_move_eol(mark_t* self) {
    return mark_set_pos_ex(self, self->bline, self->bline->char_count, 1);
}

/** TODO mark_move_bof */
int mark_move_bof(mark_t* self) {
    return mark_set_pos_ex(self, self->bline->buffer->first_line, 0, 1);
}

/** TODO mark_move_eof */
int mark_move_eof(mark_t* self) {
    return mark_set_pos_ex(self, self->bline->buffer->last_line, self->bline->buffer->last_line->char_count, 1);
}

/** TODO mark_move_next_str */
int mark_move_next_str(mark_t* self, char* match, int match_len) {
}

/** TODO mark_move_prev_str */
int mark_move_prev_str(mark_t* self, char* match, int match_len) {
}

/** TODO mark_move_next_rex */
int mark_move_next_rex(mark_t* self, char* regex, int regex_len) {
}

/** TODO mark_move_prev_rex */
int mark_move_prev_rex(mark_t* self, char* regex, int regex_len) {
}

/** TODO mark_move_bracket */
int mark_move_bracket(mark_t* self) {
}

/** TODO mark_find_next_str */
int mark_find_next_str(mark_t* self, char* match, int match_len, bline_t** ret_bline, size_t* ret_offset) {
}

/** TODO mark_find_prev_str */
int mark_find_prev_str(mark_t* self, char* match, int match_len, bline_t** ret_bline, size_t* ret_offset) {
}

/** TODO mark_find_next_rex */
int mark_find_next_rex(mark_t* self, char* regex, int regex_len, bline_t** ret_bline, size_t* ret_offset, size_t* ret_length) {
}

/** TODO mark_find_prev_rex */
int mark_find_prev_rex(mark_t* self, char* regex, int regex_len, bline_t** ret_bline, size_t* ret_offset, size_t* ret_length) {
}

/** TODO mark_find_bracket */
int mark_find_bracket(mark_t* self, bline_t** ret_bline, size_t* ret_offset) {
}

/** Move mark to bline:char_offset */
int mark_set_pos(mark_t* self, bline_t* bline, size_t char_offset) {
    return mark_set_pos_ex(self, bline, char_offset, 1);
}

/** Move mark to boffset */
int mark_set_boffset(mark_t* self, size_t boffset) {
    bline_t* bline;
    size_t char_offset;
    buffer_sanitize_position(self->bline->buffer, NULL, 0, boffset, &bline, &char_offset, NULL);
    return mark_set_pos(self, bline, char_offset);
}

/** TODO mark_set_name */
int mark_set_name(mark_t* self, char* name, int name_len) {
}

/** Destroy a mark */
int mark_destroy(mark_t* self) {
    ATTO_RETURN_OK;
}

// ============================================================================

static void bline_add_mark(bline_t* bline, mark_t* mark) {
    mark_node_t* mark_node;

    // Add to bline's list
    mark_node = calloc(1, sizeof(mark_node_t));
    mark_node->mark = mark;
    DL_APPEND(bline->mark_nodes, mark_node);

    // Add to buffer's list
    mark_node = calloc(1, sizeof(mark_node_t));
    mark_node->mark = mark;
    DL_APPEND(bline->buffer->mark_nodes, mark_node);
}

/** Move mark to bline:char_offset */
static int mark_set_pos_ex(mark_t* self, bline_t* bline, size_t char_offset, int set_target_char_offset) {
    size_t boffset;
    bline_t* orig_bline;
    mark_node_t* tmp_mark_node;

    orig_bline = self->bline;
    buffer_sanitize_position(bline->buffer, bline, char_offset, 0, &bline, &char_offset, &boffset);

    self->bline = bline;
    self->char_offset = char_offset;
    if (set_target_char_offset) {
        self->target_char_offset = char_offset;
    }
    self->boffset = boffset;

    if (orig_bline != bline) {
        tmp_mark_node = NULL;
        DL_SEARCH_SCALAR(orig_bline->mark_nodes, tmp_mark_node, mark, self);
        DL_DELETE(orig_bline->mark_nodes, tmp_mark_node);
        DL_APPEND(bline->mark_nodes, tmp_mark_node);
    }

    if (self->cursor && self->cursor->bview->active_cursor->mark == self) {
        bview_update_viewport(self->cursor->bview, self);
    }

    ATTO_RETURN_OK;
}

/** Center bview viewport around a mark */
static void bview_update_viewport(bview_t* self, mark_t* mark) {
    bview_update_viewport_dimension(self, mark->char_offset /* TODO this is broken for tabs; need screen_offset */, self->viewport_scope_x, self->rect_buffer.w, self->viewport_x, &(self->viewport_x));
    bview_update_viewport_dimension(self, mark->bline->line_index, self->viewport_scope_y, self->rect_buffer.h, self->viewport_y, &(self->viewport_y));
}

/** Called by bview_update_viewport for width and height */
static void bview_update_viewport_dimension(bview_t* self, size_t cursor_coord, ssize_t viewport_scope, ssize_t viewport_h, ssize_t viewport_y, ssize_t* ret_viewport_y) {
    ssize_t scope_start;
    ssize_t scope_stop;
    ssize_t scope;
    ssize_t coord;

    if (viewport_scope < 0) {
        scope = ATTO_MAX(viewport_scope, (viewport_h * -1));
        scope_start = viewport_y - scope;
        scope_stop = (viewport_y + viewport_h) + scope;
    } else {
        scope = ATTO_MIN(viewport_scope, viewport_h);
        scope_start = (viewport_y + (viewport_h / 2)) - (int)floorf((float)scope * 0.5);
        scope_stop = (viewport_y + (viewport_h / 2)) + (int)ceilf((float)scope * 0.5);
    }

    coord = (ssize_t)cursor_coord;
    if (coord < scope_start) {
        *ret_viewport_y = viewport_y - (scope_start - coord);
    } else if (coord >= scope_stop) {
        *ret_viewport_y = viewport_y + ((coord - scope_stop) + 1);
    }

    if (*ret_viewport_y < 0) {
        *ret_viewport_y = 0; // TODO make negative viewport an option
    }
}
