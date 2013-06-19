#include "atto.h"

int mark_get(mark_t* self, int* ret_line, int* ret_col) {
    if (ret_line) {
        *ret_line = self->line;
    }
    if (ret_col) {
        *ret_col = self->col;
    }
    return ATTO_RC_OK;
}

int mark_set(mark_t* self, int offset) {
    int line;
    int col;
    buffer_get_line_col(self->buffer, offset, &line, &col);
    self->line = line;
    self->col = col;
    self->offset = buffer_get_offset(self->buffer, line, col);
    return ATTO_RC_OK;
}

int mark_set_line_col(mark_t* self, int line, int col) {
    mark_set(self, buffer_get_offset(self->buffer, line, col));
}

int mark_move(mark_t* self, int delta) {
    return mark_set(self, self->offset + delta);
}

int mark_move_line(mark_t* self, int line_delta) {
    int offset;
    offset = buffer_get_offset(self->buffer, self->line + line_delta, self->target_col);
    mark_set(self, offset);
}
