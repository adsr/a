bview_t* bview_new() {
    return NULL;
}

int bview_update(bview_t* self) {
    return ATTO_RC_ERR;
}

int bview_move(bview_t* self, int x, int y) {
    return ATTO_RC_ERR;
}

int bview_resize(bview_t* self, int w, int h) {
    return ATTO_RC_ERR;
}

int bview_viewport_move(bview_t* self, int line_delta, int col_delta) {
    return ATTO_RC_ERR;
}

int bview_viewport_set(bview_t* self, int line, int col) {
    return ATTO_RC_ERR;
}

bview_t* bview_split(bview_t* self, int is_vertical, float factor) {
    return NULL;
}

int bview_keymap_push(bview_t* self, keymap_t* keymap) {
    return ATTO_RC_ERR;
}

keymap_t* bview_keymap_pop(bview_t* self) {
    return NULL;
}

int bview_set_active(bview_t* self) {
    return ATTO_RC_ERR;
}

bview_t* bview_get_active() {
    return NULL;
}

int bview_destroy(bview_t* self) {
    return ATTO_RC_ERR;
}
