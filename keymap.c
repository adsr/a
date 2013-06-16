#include "atto.h"

keymap_t* keymap_new(int is_fall_through_allowed) {
    keymap_t* keymap;
    keymap = (keymap_t*)calloc(1, sizeof(keymap_t));
    keymap->default_fn = LUA_REFNIL;
    keymap->is_fall_through_allowed = is_fall_through_allowed;
    return keymap;
}

int keymap_bind(keymap_t* self, char* keyc, int fn_handler) {
    kbinding_t* binding;
    ATTO_DEBUG_PRINTF("Binding %s to %d\n", keyc, fn_handler);
    binding = kbinding_new(keyc, fn_handler);
    HASH_ADD_STR(self->bindings, keyc, binding);
    return ATTO_RC_OK;
}

int keymap_bind_default(keymap_t* self, int fn_handler) {
    ATTO_DEBUG_PRINTF("Binding default to %d\n", fn_handler);
    self->default_fn = fn_handler;
    return ATTO_RC_OK;
}

int keymap_destroy(keymap_t* self) {
    kbinding_t* binding;
    kbinding_t* tmp;
    HASH_ITER(hh, self->bindings, binding, tmp) {
        kbinding_destroy(binding);
    }
    free(self);
    return ATTO_RC_OK;
}
