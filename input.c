#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "input.h"

key_trie_node_t* key_trie_root = NULL;

char* input_get_keychord(int(*getch)()) {
    int code;
    key_trie_node_t* cur_node = key_trie_root->child;

    if (cur_node == NULL) {
        return NULL;
    }

    while(1) {
        code = (*getch)();
        while (cur_node != NULL && cur_node->code != code) {
            cur_node = cur_node->sibling;
            if (cur_node == NULL) {
                break;
            }
        }
        if (cur_node == NULL || cur_node->child == NULL) {
            break;
        }
        cur_node = cur_node->child;
    }

    return cur_node != NULL ? cur_node->keychord : NULL;
}

void input_init() {
    int code;
    key_trie_node_t* meta_root;
    key_trie_node_t* bracket_root;
    key_trie_node_t* big_o_root;

    key_trie_root = (key_trie_node_t*)calloc(1, sizeof(key_trie_node_t));

    // 1-26: ctrl-[a-z]
    for (code = 1; code <= 26; code++) {
        if (code == 10) {
            key_trie_add(key_trie_root, code, "enter");
        } else if (code == 8) {
            key_trie_add(key_trie_root, code, "backspace");
        } else if (code == 9) {
            key_trie_add(key_trie_root, code, "tab");
        } else {
            key_trie_add(key_trie_root, code, "ctrl-%c", (char)('a' - 1 + code));
        }
    }

    // 27: alt-*
    meta_root = key_trie_add(key_trie_root, 27, "alt");

        // 27.32 alt-space
        key_trie_add(meta_root, 32, "alt-space");

        // 27.33-126: alt-<printable ASCII>
        for (code = 33; code <= 126; code++) {
            if (code == '[') {
                continue;
            }
            key_trie_add(meta_root, code, "alt-%c", (char)code);
        }

        // 27.91
        bracket_root = key_trie_add(meta_root, '[', "alt-[");
            key_trie_add(bracket_root, 'A', "up");
            key_trie_add(bracket_root, 'B', "down");
            key_trie_add(bracket_root, 'C', "left");
            key_trie_add(bracket_root, 'D', "right");
            key_trie_add(bracket_root, 'E', "5");
            key_trie_add(bracket_root, 'F', "end");
            key_trie_add(bracket_root, 'G', "5");
            key_trie_add(bracket_root, 'H', "home");
            key_trie_add(bracket_root, 'M', "mouse");
            key_trie_add(bracket_root, 'Z', "shift-tab");
        big_o_root = key_trie_add(meta_root, 'O', "alt-O");
            key_trie_add(big_o_root, 'A', "up");
            key_trie_add(big_o_root, 'B', "down");
            key_trie_add(big_o_root, 'C', "left");
            key_trie_add(big_o_root, 'D', "right");
            key_trie_add(big_o_root, 'H', "home");
            key_trie_add(big_o_root, 'F', "end");

    // 28-31: ctrl+f[4-7]
    for (code = 28; code <= 31; code++) {
        key_trie_add(key_trie_root, code, "ctrl-f%d", code - 24);
    }

    // 32: space
    key_trie_add(key_trie_root, 32, "space");

    // 33-126: printable ASCII
    for (code = 33; code <= 126; code++) {
        key_trie_add(key_trie_root, code, "%c", (char)code);
    }

    // misc
    key_trie_add(key_trie_root, 127, "backspace");
    key_trie_add(key_trie_root, 258, "down");
    key_trie_add(key_trie_root, 259, "up");
    key_trie_add(key_trie_root, 260, "left");
    key_trie_add(key_trie_root, 261, "right");
    key_trie_add(key_trie_root, 262, "home");
    key_trie_add(key_trie_root, 263, "backspace");
    key_trie_add(key_trie_root, 330, "delete");
    key_trie_add(key_trie_root, 331, "insert");
    key_trie_add(key_trie_root, 338, "page-down");
    key_trie_add(key_trie_root, 339, "page-up");
    key_trie_add(key_trie_root, 343, "enter");
    key_trie_add(key_trie_root, 350, "5");
    key_trie_add(key_trie_root, 360, "end");
    key_trie_add(key_trie_root, 410, "resize");
}


key_trie_node_t* key_trie_add(key_trie_node_t* parent, int code, const char* keychord_format, ...) {
    key_trie_node_t* node = (key_trie_node_t*)calloc(1, sizeof(key_trie_node_t));
    key_trie_node_t* cur = NULL;
    va_list args;

    va_start(args, keychord_format);
    va_end(args);

    node->code = code;
    vsprintf(node->keychord, keychord_format, args);

    if (parent->child == NULL) {
        parent->child = node;
    } else {
        cur = parent->child;
        while (1) {
            if (cur->sibling == NULL) {
                break;
            }
            cur = cur->sibling;
        }
        cur->sibling = node;
    }

    return node;
}
