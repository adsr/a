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
}

int mark_move(mark_t* self, int delta) {
}
