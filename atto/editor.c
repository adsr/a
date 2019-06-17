#include "atto.h"

static void editor_loop();
static int editor_try_exec_function_for_input(struct tb_event* ev);
static void editor_draw();
static void editor_resize(int w, int h);
static void editor_get_input(struct tb_event* ev);
static int editor_bview_exists(bview_t* bview);
static void editor_free();
static void editor_init_prompt();
static void editor_init_default_keymap();
static ATTO_FUNCTION(editor_default_keymap_handler);

editor_t* editor = NULL;

/** Open a bview and set it as active */
int editor_open_bview(char* opt_filename, int opt_filename_len, bview_t* opt_before, int make_active, bview_t** optret_bview) {
    bview_t* bview;
    int rc;
    if (!opt_filename) {
        bview_new(NULL, 0, &bview);
    } else if ((rc = bview_new(opt_filename, opt_filename_len, &bview)) != ATTO_RC_OK) {
        return rc;
    }
    if (opt_before) {
        if (!editor_bview_exists(opt_before)) {
            ATTO_RETURN_ERR("No bview %p in editor->bviews", opt_before);
        }
        DL_PREPEND_ELEM(editor->bviews, opt_before, bview);
    } else {
        DL_APPEND(editor->bviews, bview);
    }
    if (make_active) {
        editor_set_active(bview);
    }
    if (optret_bview) {
        *optret_bview = bview;
    }
    ATTO_RETURN_OK;
}

/** Close a bview and destroy it */
int editor_close_bview(bview_t* bview) {
    bview_t* prev;
    prev = bview->prev;
    if (!editor_bview_exists(bview)) {
        ATTO_RETURN_ERR("No bview %p in editor->bviews", bview);
    }
    DL_DELETE(editor->bviews, bview);
    bview_destroy(bview);
    if (prev) {
        editor_set_active(prev);
    } else {
        editor_open_bview(NULL, 0, NULL, 1, NULL);
    }
    ATTO_RETURN_OK;
}

/** Return the number of open bviews */
int editor_get_num_bviews(int* ret_count) {
    bview_t* tmp;
    int count;
    count = 0;
    DL_FOREACH(editor->bviews, tmp) {
        count += 1;
    }
    *ret_count = count;
    ATTO_RETURN_OK;
}

/** Return the n-th open bview */
int editor_get_bview(int bview_num, bview_t** ret_bview) {
    bview_t* tmp;
    int n;
    n = 0;
    DL_FOREACH(editor->bviews, tmp) {
        if (n == bview_num) {
            *ret_bview = tmp;
            ATTO_RETURN_OK;
        }
        n += 1;
    }
    ATTO_RETURN_ERR("bview_num %d does not exist", bview_num);
}

/** Getter for status */
int editor_get_status(char** ret_status, int* ret_status_len) {
    *ret_status = editor->status;
    *ret_status_len = editor->status_len;
    ATTO_RETURN_OK;
}

/** Setter for status */
int editor_set_status(char* status, int status_len) {
    editor->status = strndup(status, status_len);
    editor->status_len = strlen(editor->status);
    ATTO_RETURN_OK;
}

/** Set the active bview */
int editor_set_active(bview_t* bview) {
    if (!editor_bview_exists(bview)) {
        ATTO_RETURN_ERR("No bview %p in editor->bviews", bview);
    }
    editor->active = bview;
    if (bview != editor->prompt) {
        editor->edit = bview;
    }
    ATTO_RETURN_OK;
}

/** Get the active bview */
int editor_get_active(bview_t** ret_bview) {
    *ret_bview = editor->active;
    ATTO_RETURN_OK;
}

/** Setter for is_render_disabled */
int editor_set_render_disabled(int is_render_disabled) {
    editor->is_render_disabled = (is_render_disabled ? 1 : 0);
    ATTO_RETURN_OK;
}

/** Getter for is_render_disabled */
int editor_get_render_disabled(int* ret_is_render_disabled) {
    *ret_is_render_disabled = (editor->is_render_disabled ? 1 : 0);
    ATTO_RETURN_OK;
}

/** Setter for should_exit */
int editor_set_should_exit(int should_exit) {
    editor->should_exit = (should_exit ? 1 : 0);
    ATTO_RETURN_OK;
}

/** Getter for should_exit */
int editor_get_should_exit(int* ret_should_exit) {
    *ret_should_exit = (editor->should_exit ? 1 : 0);
    ATTO_RETURN_OK;
}

// ============================================================================

/** Program entry point */
int main(int argc, char** argv) {
    // TODO argv: -h, -v, file:lineno, etc
    // TODO rc: ~/.atto/
    // TODO hooks
    bview_t* bview;

    tb_init();
    tb_select_input_mode(TB_INPUT_ALT);

    editor = calloc(1, sizeof(editor_t));

    editor_init_default_keymap();
    editor_init_prompt();
    editor->rect_status.bg = TB_REVERSE;
    editor_open_bview("/home/adam/atto/test.txt", strlen("/home/adam/atto/test.txt"), NULL, 1, &bview);
    editor_resize(tb_width(), tb_height());
    editor_draw();
    editor_loop();
    editor_free();

    tb_shutdown();
}

/** Editor loop */
static void editor_loop() {
    struct tb_event ev;
    int rc;

    // Loop until exit flag is set
    do {
        // Get input
        editor_get_input(&ev);

        if (ev.type == TB_EVENT_RESIZE) {
            // Resize
            editor_resize(ev.w, ev.h);
        } else {
            // Key
            // TODO hook input
            if ((rc = editor_try_exec_function_for_input(&ev)) != ATTO_RC_OK) {
                // TODO hook error
            }
        }

        if (!editor->is_render_disabled) {
            // Draw cursor

            // Draw
            editor_draw();
        }
    } while (!editor->should_exit && ev.ch!='q');
}

/** Execute a function for a given input */
static int editor_try_exec_function_for_input(struct tb_event* ev) {
    kbinding_t* kbinding;
    keyinput_t keyinput;
    keymap_node_t* keymap_node;

    kbinding = NULL;
    keyinput.mod = ev->mod;
    keyinput.ch = ev->ch;
    keyinput.key = ev->key;
    keymap_node = editor->active->keymap_node_tail;

    // Look up function for input on active bview
    while (keymap_node) {
        HASH_FIND(hh, keymap_node->keymap->bindings, &keyinput, sizeof(keyinput_t), kbinding);
        if (kbinding) {
            // Execute function
            return kbinding->function(ATTO_FUNCTION_ARGS(editor, &keyinput));
        } else if (keymap_node->keymap->default_function) {
            // Execute default function
            return keymap_node->keymap->default_function(ATTO_FUNCTION_ARGS(editor, &keyinput));
        } else if (!keymap_node->keymap->is_fallthrough_allowed) {
            // Cannot fall-through, so break
            break;
        }
        // Fall-through to prev keymap
        keymap_node = keymap_node->prev;
    }
    ATTO_RETURN_ERR("No binding for keyinput{.mod=%u .ch=%u .key=%u}", keyinput.mod, keyinput.ch, keyinput.key);
}

/** Draw the editor */
static void editor_draw() {
    int cx;
    int cy;

    tb_clear();

    // Draw cursor
    cx = 0;
    cy = 0;
    bview_get_absolute_cursor_coords(editor->active, &cx, &cy);
    tb_set_cursor(cx, cy);

    // Draw edit buffer
    bview_draw(editor->edit);

    // Draw prompt buffer
    bview_draw(editor->prompt);

    // Draw status
    // TODO status colors
    tb_printf(editor->rect_status, 0, 0, 0, 0, "%-*.*s", editor->rect_status.w, editor->rect_status.w, editor->status);

    tb_present();
}

/** Resize the editor */
static void editor_resize(int w, int h) {
    bview_t* bview;
    editor->width = w;
    editor->height = h;
    bview = NULL;
    DL_FOREACH(editor->bviews, bview) {
        if (bview != editor->prompt) {
            bview_resize(bview, 0, 0, editor->width, editor->height - 2);
        }
    }
    bview_resize(editor->prompt, 0, editor->height - 1, editor->width, 1);

    editor->rect_status.x = 0;
    editor->rect_status.y = editor->height - 2;
    editor->rect_status.w = w;
    editor->rect_status.h = 1;
}

/** Get user input, drawing from keyqueue if not empty */
static void editor_get_input(struct tb_event* ev) {
    keyqueue_node_t* tail;
    keyqueue_node_t* tmp;
    tail = editor->keyqueue_node_tail;
    if (tail) {
        ev->type = TB_EVENT_KEY;
        ev->mod = (tail->keyinput).mod;
        ev->key = (tail->keyinput).key;
        ev->ch = (tail->keyinput).ch;
        tmp = tail->prev;
        DL_DELETE(editor->keyqueue_nodes, tail);
        free(tail);
        editor->keyqueue_node_tail = tmp;
    } else {
        tb_poll_event(ev);
    }
}

/** Return 1 if bview exists in editor's list of bviews */
static int editor_bview_exists(bview_t* bview) {
    bview_t* tmp;
    DL_FOREACH(editor->bviews, tmp) {
        if (tmp == bview) {
            return 1;
        }
    }
    return 0;
}

/** Free the editor */
static void editor_free() {
    bview_t* bview;
    bview_t* bview_tmp;
    buffer_t* buffer;
    buffer_t* buffer_tmp;
    keyqueue_node_t* keyqueue_node;
    keyqueue_node_t* keyqueue_node_tmp;

    DL_FOREACH_SAFE(editor->bviews, bview, bview_tmp) {
        DL_DELETE(editor->bviews, bview);
        bview_destroy(bview);
    }

    DL_FOREACH_SAFE(editor->buffers, buffer, buffer_tmp) {
        DL_DELETE(editor->buffers, buffer);
        buffer_destroy(buffer);
    }

    DL_FOREACH_SAFE(editor->keyqueue_nodes, keyqueue_node, keyqueue_node_tmp) {
        DL_DELETE(editor->keyqueue_nodes, keyqueue_node);
        free(keyqueue_node);
    }

    keymap_destroy(editor->default_keymap);

    if (editor->status) free(editor->status);
    if (editor->last_error) free(editor->last_error);
}

/** Init editor->prompt */
static void editor_init_prompt() {
    editor_open_bview(NULL, 0, NULL, 0, &(editor->prompt));
    editor->prompt->is_chromeless = 1;
}

/** Init editor->default_keymap */
static void editor_init_default_keymap() {
    // TODO
    keymap_new(0, &(editor->default_keymap));
    keymap_bind_default(editor->default_keymap, editor_default_keymap_handler);
}

static ATTO_FUNCTION(editor_default_keymap_handler) {
    int ucstr_len;
    char ucstr[8];

    //fprintf(stderr, "key=%d ch=%d mod=%d\n", keyinput->key, keyinput->ch, keyinput->mod);

    // TODO
    // http://tnerual.eriogerg.free.fr/vimqrc.html
    // https://ccrma.stanford.edu/guides/package/emacs/emacs.html
    // http://sam.cat-v.org/cheatsheet/

    if (keyinput->ch && !keyinput->mod) {
        ucstr_len = tb_utf8_unicode_to_char((char*)ucstr, keyinput->ch);
        cursor_insert(cursor, (char*)ucstr, ucstr_len);
    } else if (keyinput->key) {
        switch (keyinput->key) {
            case TB_KEY_SPACE: cursor_insert(cursor, (char*)" ", 1); break;
            case TB_KEY_ENTER: cursor_insert(cursor, (char*)"\n", 1); break;
            case TB_KEY_DELETE: cursor_delete(cursor, 1); break;
            case TB_KEY_BACKSPACE:
                if (cursor->mark->boffset > 0) {
                    cursor_move(cursor, -1);
                    cursor_delete(cursor, 1);
                }
                break;
            case TB_KEY_ARROW_LEFT: cursor_move(cursor, -1); break;
            case TB_KEY_ARROW_RIGHT: cursor_move(cursor, 1); break;
            case TB_KEY_ARROW_UP: cursor_move_vert(cursor, -1); break;
            case TB_KEY_ARROW_DOWN: cursor_move_vert(cursor, 1); break;
            case TB_KEY_CTRL_E: cursor_move_eol(cursor); break;
            case TB_KEY_CTRL_A: cursor_move_bol(cursor); break;
        }
    }

    ATTO_RETURN_OK;
}
