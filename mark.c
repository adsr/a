#include "atto.h"

/**
 * Retrieve the offset, line, and col of a mark
 */
int mark_get(mark_t* self, int* ret_line, int* ret_col) {
    if (ret_line) {
        *ret_line = self->line;
    }
    if (ret_col) {
        *ret_col = self->col;
    }
    return self->offset;
}

/**
 * Set by offset
 */
int mark_set(mark_t* self, int offset) {
    int line;
    int col;
    buffer_get_line_col(self->buffer, offset, &line, &col);
    self->line = line;
    self->col = col;
    self->target_col = col;
    self->offset = buffer_get_offset(self->buffer, line, col);
    if (g_bview_active && g_bview_active->cursor == self) {
        _bview_update_viewport(g_bview_active, line, col);
        bview_update(g_bview_active);
    }
    return ATTO_RC_OK;
}

/**
 * Set by line and col
 */
int mark_set_line_col(mark_t* self, int line, int col) {
    mark_set(self, buffer_get_offset(self->buffer, line, col));
}

/**
 * Move by offset delta
 */
int mark_move(mark_t* self, int delta) {
    return mark_set(self, self->offset + delta);
}

/**
 * Move by line delta
 */
int mark_move_line(mark_t* self, int line_delta) {
    int offset;
    int target_col;
    target_col = self->target_col;
    offset = buffer_get_offset(self->buffer, self->line + line_delta, target_col);
    mark_set(self, offset);
    self->target_col = target_col; // Restore original target_col
}
