mark_t* mark_new(bview_t* bview, bline_t* line, int col) {
    return NULL;
}

int mark_get(mark_t* self, bline_t** ret_line, int* ret_col) {
    return ATTO_RC_ERR;
}

int mark_set(mark_t* self, bline_t* line, int col) {
    return ATTO_RC_ERR;
}

int mark_move(mark_t* self, int delta) {
    return ATTO_RC_ERR;
}

int mark_destroy(mark_t* self) {
    return ATTO_RC_ERR;
}
