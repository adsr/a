srule_t* srule_new() {
    return NULL;
}

srule_t* srule_add_single(char* regex) {
    return NULL;
}

srule_t* srule_add_multi(char* regex_start, char* regex_end) {
    return NULL;
}

srule_t* srule_add_range(mark_t* start, mark_t* end) {
    return NULL;
}

int srule_destroy(srule_t* self) {
    return ATTO_RC_ERR;
}
