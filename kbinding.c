#include "atto.h"

/**
 * Allocate a new key binding
 */
kbinding_t* kbinding_new(char* keyc, int fn) {
    kbinding_t* kbinding;
    kbinding = (kbinding_t*)calloc(1, sizeof(kbinding_t));
    snprintf(kbinding->keyc, ATTO_KEYC_LEN, "%s", keyc);
    kbinding->fn = fn;
    return kbinding;
}

/**
 * Free a keybinding
 */
int kbinding_destroy(kbinding_t* self) {
    free(self);
}
