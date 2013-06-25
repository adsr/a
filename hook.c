#include "atto.h"

hook_t* hook_new(char* name, int fn_handler) {
    hook_t* hook;
    hook = (hook_t*)calloc(1, sizeof(hook_t));
    hook->name = strdup(name);
    hook->ref = fn_handler;
    return hook;
}

int _hook_notify(char* name, int num_args) {
    // TODO
    return ATTO_RC_OK;
}

int hook_destroy(hook_t* self) {
    // TODO
    return ATTO_RC_OK;
}
