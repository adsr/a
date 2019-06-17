#include "atto.h"

#define ATTO_INVOKE_MARK_FUNC(cursor, mark_func) do { \
    int rc; \
    rc = (mark_func)((cursor)->mark); \
    if (rc != ATTO_RC_OK) return rc; \
    if (!(cursor)->is_sel_bound_anchored) { \
        rc = (mark_func)((cursor)->sel_bound); \
    } \
    return rc; \
} while(0)

#define ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(cursor, mark_func, ...) do { \
    int rc; \
    rc = (mark_func)((cursor)->mark, __VA_ARGS__); \
    if (rc != ATTO_RC_OK) return rc; \
    if (!(cursor)->is_sel_bound_anchored) { \
        rc = (mark_func)((cursor)->sel_bound, __VA_ARGS__); \
    } \
    return rc; \
} while(0)

/** Create a new cursor */
int cursor_new(bview_t* bview, bline_t* bline, size_t offset, int is_asleep, cursor_t** ret_cursor) {
    cursor_t* cursor;

    cursor = calloc(1, sizeof(cursor_t));
    cursor->bview = bview;

    mark_new(bline, offset, "", 0, cursor, &(cursor->mark));
    mark_new(bline, offset, "", 0, cursor, &(cursor->sel_bound));
    cursor->is_asleep = (is_asleep ? 1 : 0);

    *ret_cursor = cursor;
    ATTO_RETURN_OK;
}

/** TODO cursor_anchor_sel_bound */
int cursor_anchor_sel_bound(cursor_t* self) {
}

/** TODO cursor_retract_sel_bound */
int cursor_retract_sel_bound(cursor_t* self) {
}

/** TODO cursor_swap_mark_with_sel_bound */
int cursor_swap_mark_with_sel_bound(cursor_t* self) {
}

/** Insert str at cursor */
int cursor_insert(cursor_t* self, char* str, int str_len) {
    return buffer_insert(self->mark->bline, self->mark->char_offset, str, str_len, 1);
}

/** TODO cursor_replace */
int cursor_replace(cursor_t* self, char* str, int str_len) {
}

/** TODO cursor_backspace */
int cursor_backspace(cursor_t* self, size_t num_chars) {
}

/** Delete num_chars after cursor */
int cursor_delete(cursor_t* self, size_t num_chars) {
    return buffer_delete(self->mark->bline, self->mark->char_offset, num_chars, 1);
}

/** TODO cursor_wake_up */
int cursor_wake_up(cursor_t* self) {
}

/** TODO cursor_fall_asleep */
int cursor_fall_asleep(cursor_t* self) {
}

/** TODO cursor_move */
int cursor_move(cursor_t* self, ssize_t char_delta) {
    ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(self, mark_move, char_delta);
}

/** TODO cursor_move_vert */
int cursor_move_vert(cursor_t* self, ssize_t num_lines) {
    ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(self, mark_move_vert, num_lines);
}

/** TODO cursor_move_bol */
int cursor_move_bol(cursor_t* self) {
    ATTO_INVOKE_MARK_FUNC(self, mark_move_bol);
}

/** TODO cursor_move_eol */
int cursor_move_eol(cursor_t* self) {
    ATTO_INVOKE_MARK_FUNC(self, mark_move_eol);
}

/** TODO cursor_move_bof */
int cursor_move_bof(cursor_t* self) {
    ATTO_INVOKE_MARK_FUNC(self, mark_move_bof);
}

/** TODO cursor_move_eof */
int cursor_move_eof(cursor_t* self) {
    ATTO_INVOKE_MARK_FUNC(self, mark_move_eof);
}

/** TODO cursor_move_next_str */
int cursor_move_next_str(cursor_t* self, char* match, int match_len) {
    ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(self, mark_move_next_str, match, match_len);
}

/** TODO cursor_move_prev_str */
int cursor_move_prev_str(cursor_t* self, char* match, int match_len) {
    ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(self, mark_move_prev_str, match, match_len);
}

/** TODO cursor_move_next_rex */
int cursor_move_next_rex(cursor_t* self, char* regex, int regex_len) {
    ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(self, mark_move_next_rex, regex, regex_len);
}

/** TODO cursor_move_prev_rex */
int cursor_move_prev_rex(cursor_t* self, char* regex, int regex_len) {
    ATTO_INVOKE_MARK_FUNC_WITH_PARAMS(self, mark_move_prev_rex, regex, regex_len);
}

/** TODO cursor_move_bracket */
int cursor_move_bracket(cursor_t* self) {
    ATTO_INVOKE_MARK_FUNC(self, mark_move_bracket);
}

/** TODO cursor_destroy */
int cursor_destroy(cursor_t* self) {
    ATTO_RETURN_OK;
}

// ============================================================================
