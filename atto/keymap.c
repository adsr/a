#include "atto.h"

/** Create a new keymap */
int keymap_new(int is_fallthrough_allowed, keymap_t** ret_keymap) {
    keymap_t* self;
    self = calloc(1, sizeof(keymap_t));
    self->is_fallthrough_allowed = (is_fallthrough_allowed ? 1 : 0);
    *ret_keymap = self;
    ATTO_RETURN_OK;
}

/** Setter for is_fallthrough_allowed */
int keymap_set_is_fallthrough_allowed(keymap_t* self, int is_fallthrough_allowed) {
    self->is_fallthrough_allowed = (is_fallthrough_allowed ? 1 : 0);
    ATTO_RETURN_OK;
}

/** Getter for is_fallthrough_allowed */
int keymap_get_is_fallthrough_allowed(keymap_t* self, int *ret_is_fallthrough_allowed) {
    *ret_is_fallthrough_allowed = (self->is_fallthrough_allowed ? 1 : 0);
    ATTO_RETURN_OK;
}

/** Bind a function to a keyinput */
int keymap_bind(keymap_t* self, keyinput_t keyinput, keymap_function_t function) {
    kbinding_t* kbinding;
    kbinding = calloc(1, sizeof(kbinding_t));
    (kbinding->keyinput).mod = keyinput.mod;
    (kbinding->keyinput).ch = keyinput.ch;
    (kbinding->keyinput).key = keyinput.key;
    kbinding->function = function;
    HASH_ADD(hh, self->bindings, keyinput, sizeof(keyinput_t), kbinding);
    ATTO_RETURN_OK;
}

/** Unbind a keyinput */
int keymap_unbind(keymap_t* self, keyinput_t keyinput) {
    kbinding_t* kbinding;
    kbinding = NULL;
    HASH_FIND(hh, self->bindings, &keyinput, sizeof(keyinput_t), kbinding);
    if (!kbinding) {
        ATTO_RETURN_ERR("Binding for keyinput{mod=%u ch=%u key=%u} does not exist", keyinput.mod, keyinput.ch, keyinput.key);
    }
    HASH_DEL(self->bindings, kbinding);
    ATTO_RETURN_OK;
}

/** Bind a default function */
int keymap_bind_default(keymap_t* self, keymap_function_t function) {
    self->default_function = function;
    ATTO_RETURN_OK;
}

/** Unbind default function */
int keymap_unbind_default(keymap_t* self) {
    self->default_function = NULL;
    ATTO_RETURN_OK;
}

/** Destroy a keymap */
int keymap_destroy(keymap_t* self) {
    kbinding_t* kbinding;
    kbinding_t* tmp;
    HASH_ITER(hh, self->bindings, kbinding, tmp) {
        HASH_DEL(self->bindings, kbinding);
        free(kbinding);
    }
    ATTO_RETURN_OK;
}

// ============================================================================
