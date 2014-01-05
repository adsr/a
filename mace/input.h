#ifndef _INPUT_H
#define _INPUT_H

#define MAX_INPUT_CODE_LEN 32

typedef struct key_trie_node_s {
    int code;
    char keychord[64];
    struct key_trie_node_s* child;
    struct key_trie_node_s* sibling;
} key_trie_node_t;

typedef struct keychord_s {
    char* name;
    int* codes;
    int code_count;
    char ascii[2];
} keychord_t;

keychord_t* input_get_keychord(int(*getch)());

void input_init();

key_trie_node_t* key_trie_add(key_trie_node_t* parent, int code, const char* keychord_format, ...);

#endif
