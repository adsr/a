keymap_t* keymap_new() {
    return NULL;
}

int keymap_bind(keymap_t* self, char* keyc, int fn_handler) {
    return ATTO_RC_ERR;
}


int keymap_bind_default(keymap_t* self, int fn_handler) {
    return ATTO_RC_ERR;
}

int keymap_destroy(keymap_t* self) {
    return ATTO_RC_ERR;
}
