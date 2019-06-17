#include "atto.h"

// TODO standardize identifiers: bline, line_index, offset, boffset, xoffset, bxoffset, ret_*, opt_*, optret_*
// TODO find and fix size_t + ssize_t expressions

static bline_t* bline_new(buffer_t* parent, char* data, size_t data_len, size_t line_index, baction_t* baction);
static void bline_update_char_count(bline_t* self);
static void bline_update_char_widths(bline_t* self);
static void buffer_update_fstat(buffer_t* self, char* fname, FILE* fp);
static void buffer_insert_data(bline_t* self, size_t char_offset, char* data, size_t data_len, baction_t* baction, int do_fill_action);
static void buffer_delete_data(bline_t* self, size_t char_offset, size_t num_chars_to_delete, baction_t* baction, int do_fill_action);
static void buffer_update_marks(buffer_t* self, baction_t* baction);
static void buffer_update_metadata(buffer_t* self, baction_t* baction);
static void buffer_update_line_indexes(buffer_t* self, bline_t* start, ssize_t line_delta);
static void bline_insert(bline_t* self, char* data, size_t data_len, size_t char_offset, baction_t* baction);
static void bline_delete(bline_t* self, size_t char_offset, size_t num_chars_to_delete, baction_t* baction);
static void bline_replace_after(bline_t* self, char* data, size_t data_len, size_t char_offset, baction_t* baction, char **ret_after, size_t* ret_after_len);
static void bline_place(bline_t* self, bline_t* before);
static void bline_destroy(bline_t* self, baction_t* baction);
static size_t bline_get_byte_from_char_offset(bline_t* self, size_t char_offset);
static baction_t* baction_new(bline_t* bline, size_t char_offset, size_t boffset, int type);
static void baction_set_data_from_range(baction_t* self, bline_t* start_bline, size_t start_offset, bline_t* end_bline, size_t end_offset);
static void baction_append(baction_t* self, char* data, size_t data_len);
static int baction_do(baction_t* self, bline_t* override_bline, size_t override_char_offset);
static int baction_undo(baction_t* self);
static void buffer_record_action(buffer_t* self, baction_t* baction);
static void baction_destroy(baction_t* self);
void buffer_print(buffer_t* self);

/** Create a new buffer */
int buffer_new(buffer_t** ret_buffer) {
    buffer_t* self;
    bline_t* first_line;
    self = calloc(1, sizeof(buffer_t));
    first_line = bline_new(self, "", 0, 0, NULL);
    DL_APPEND(self->first_line, first_line);
    self->last_line = self->first_line;
    self->line_count = 1;
    self->line_digits = 1;
    self->tab_stop = 4;
    self->byte_count = 0;
    self->char_count = 0;
    *ret_buffer = self;
    ATTO_RETURN_OK;
}

/** Convenience wrapper for buffer_new + buffer_read */
int buffer_open(char* filename, int filename_len, buffer_t** ret_buffer) {
    buffer_t* self;
    int rc;
    buffer_new(&self);
    if ((rc = buffer_read(self, filename, filename_len)) != ATTO_RC_OK) {
        buffer_destroy(self);
        return rc;
    }
    *ret_buffer = self;
    ATTO_RETURN_OK;
}

/** Read contents of filename into buffer */
int buffer_read(buffer_t* self, char* filename, int filename_len) {
    FILE* fp;
    size_t filesize;
    char* buffer;
    char* fname;

    // Opne file
    fname = strndup(filename, filename_len);
    if (!(fp = fopen(fname, "rb"))) {
        free(fname);
        ATTO_RETURN_ERR("Failed to fopen %.*s for reading", filename_len, filename);
    }

    // Read entire file
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);
    buffer = malloc(sizeof(char) * (filesize + 1)); // TODO error checking
    buffer[filesize] = '\0';
    if (fread(buffer, filesize, 1, fp) != 1) {
        free(buffer);
        free(fname);
        ATTO_RETURN_ERR("Failed to fread %.*s", filename_len, filename);
    }

    // Update fstat
    buffer_update_fstat(self, fname, fp);
    fclose(fp);

    // Set buffer
    buffer_set(self, buffer, filesize);

    ATTO_RETURN_OK;
}

/** Write contents of buffer to filename */
int buffer_write(buffer_t* self, char* filename, int filename_len, int is_append) {
    FILE* fp;
    char* fname;
    bline_t* bline;
    int fd;

    // Open file
    fname = strndup(filename, filename_len);
    if (!(fp = fopen(fname, (is_append ? "ab" : "wb")))) {
        free(fname);
        ATTO_RETURN_ERR("Failed to fopen %.*s for writing", filename_len, filename);
    }

    // Write lines
    for (bline = self->first_line; bline; bline = bline->next) {
        if (fwrite(bline->data, sizeof(char), bline->data_len, fp) != bline->data_len
            || (bline->next && fwrite("\n", sizeof(char), 1, fp) != 1)
        ) {
            fclose(fp);
            free(fname);
            ATTO_RETURN_ERR("Failed to fwrite to %.*s", filename_len, filename);
        }
    }

    // Sync to disk
    fd = fileno(fp);
    if (fsync(fd) != 0) {
        ATTO_RETURN_ERR("Failed to fsync %.*s", filename_len, filename);
    }

    // Update fstat
    buffer_update_fstat(self, fname, fp);
    fclose(fp);
    self->has_unsaved_changes = 0;

    ATTO_RETURN_OK;
}

/** Set contents of buffer to data */
int buffer_set(buffer_t* self, char* data, size_t data_len) {
    int rc;
    if ((rc = buffer_clear(self)) != ATTO_RC_OK
        || (rc = buffer_insert(self->first_line, 0, data, data_len, 0)) != ATTO_RC_OK
    ) {
        return rc;
    }
    ATTO_RETURN_OK;
}

/** Clear buffer contents */
int buffer_clear(buffer_t* self) {
    return buffer_delete(self->first_line, 0, self->char_count, 0);
}

/** Insert data into buffer // TODO DRY buffer_insert and buffer_delete */
int buffer_insert(bline_t* self, size_t char_offset, char *data, size_t data_len, int do_record_action) {
    baction_t* baction;
    size_t boffset;

    buffer_sanitize_position(self->buffer, self, char_offset, 0, &self, &char_offset, &boffset);
    baction = baction_new(self, char_offset, boffset, ATTO_BACTION_TYPE_INSERT);
    buffer_insert_data(self, char_offset, data, data_len, baction, do_record_action);

    // TODO styles
    // TODO listeners

    if (do_record_action) {
        buffer_record_action(self->buffer, baction);
    }
    ATTO_RETURN_OK;
}

/** Delete data from buffer */
int buffer_delete(bline_t* self, size_t char_offset, size_t num_chars_to_delete, int do_record_action) {
    baction_t* baction;
    size_t boffset;

    buffer_sanitize_position(self->buffer, self, char_offset, 0, &self, &char_offset, &boffset);
    baction = baction_new(self, char_offset, boffset, ATTO_BACTION_TYPE_DELETE);
    buffer_delete_data(self, char_offset, num_chars_to_delete, baction, do_record_action);

    // TODO styles
    // TODO listeners

    if (do_record_action) {
        buffer_record_action(self->buffer, baction);
    }
    ATTO_RETURN_OK;
}

/** Undo the last baction */
int buffer_undo(buffer_t* self) {
    baction_t* baction_to_undo;
    int rc;

    // Error check
    baction_to_undo = NULL;
    if (self->action_undone && self->action_undone->prev) {
        baction_to_undo = self->action_undone->prev;
    } else {
        baction_to_undo = self->action_tail;
    }
    if (!baction_to_undo) {
        ATTO_RETURN_ERR("%s", "Nothing to undo");
    }

    // Perform undo
    rc = baction_undo(baction_to_undo);
    if (rc != ATTO_RC_OK) {
        return rc;
    }

    self->action_undone = baction_to_undo;
    ATTO_RETURN_OK;
}

/** Redo the last undone baction */
int buffer_redo(buffer_t* self) {
    int rc;

    // Error check
    if (!self->action_undone) {
        ATTO_RETURN_ERR("%s", "Nothing to redo");
    }

    // Perform redo
    rc = baction_do(self->action_undone, NULL, 0);
    if (rc != ATTO_RC_OK) {
        return rc;
    }

    self->action_undone = self->action_undone->next;
    ATTO_RETURN_OK;
}

/** Repeat the last baction at a certain position */
int buffer_repeat_at(buffer_t* self, bline_t* opt_bline, size_t opt_char_offset) {
    baction_t* baction_to_repeat;
    int rc;

    if (opt_bline) {
        buffer_sanitize_position(self, opt_bline, opt_char_offset, 0, &opt_bline, &opt_char_offset, NULL);
    }

    // Error check
    baction_to_repeat = NULL;
    if (self->action_undone && self->action_undone->next)  {
        baction_to_repeat = self->action_undone->next;
    } else {
        baction_to_repeat = self->action_tail;
    }
    if (!baction_to_repeat) {
        ATTO_RETURN_ERR("%s", "Nothing to repeat");
    }

    // Perform repeat
    rc = baction_do(baction_to_repeat, opt_bline, opt_char_offset);
    if (rc != ATTO_RC_OK) {
        return rc;
    }

    ATTO_RETURN_OK;
}

/** TODO buffer_add_style */
int buffer_add_style(buffer_t* self, srule_t* style) {
    ATTO_RETURN_OK;
}

/** TODO buffer_remove_style */
int buffer_remove_style(buffer_t* self, srule_t* style) {
    ATTO_RETURN_OK;
}

/** Add a blistener to a buffer */
int buffer_add_buffer_listener(buffer_t* self, void* listener, blistener_callback_t callback, blistener_t** ret_blistener) {
    blistener_t* blistener;
    blistener = calloc(1, sizeof(blistener_t));
    blistener->listener = listener;
    blistener->callback = callback;
    DL_APPEND(self->listeners, blistener);
    *ret_blistener = blistener;
    ATTO_RETURN_OK;
}

/** Remove a blistener from a buffer */
int buffer_remove_buffer_listener(buffer_t* self, blistener_t* blistener) {
    blistener_t* elt;
    blistener_t* tmp;
    DL_FOREACH_SAFE(self->listeners, elt, tmp) {
        if (elt == blistener) {
            DL_DELETE(self->listeners, blistener);
            ATTO_RETURN_OK;
        }
    }
    ATTO_RETURN_ERR("blistener %p not found", blistener);
}

/** TODO buffer_add_mark */
int buffer_add_mark(bline_t* self, size_t offset, char* name, int name_len, mark_t** ret_mark) {
    size_t boffset;
    buffer_sanitize_position(self->buffer, self, offset, 0, &self, &offset, &boffset);
    ATTO_RETURN_OK;
}

/** TODO buffer_remove_mark */
int buffer_remove_mark(buffer_t* self, mark_t* mark) {
    ATTO_RETURN_OK;
}

/** Get a byte offset given a char offset in a bline */
int buffer_get_boffset(bline_t* self, size_t char_offset, size_t* ret_boffset) {
    size_t boffset;
    bline_t* cur_bline;
    boffset = 0;
    DL_FOREACH(self->buffer->first_line, cur_bline) {
        if (cur_bline == self) {
            boffset += char_offset;
            break;
        }
        boffset += cur_bline->char_count + 1; // Plus 1 for newline
    }
    *ret_boffset = ATTO_MIN(boffset, self->buffer->char_count);
    ATTO_RETURN_OK;
}

/** Get bline and offset given a boffset */
int buffer_get_bline_and_offset(buffer_t* self, size_t boffset, bline_t** ret_bline, size_t* ret_offset) {
    bline_t* cur_bline;
    cur_bline = self->first_line;
    while (1) {
        if (boffset <= cur_bline->char_count) {
            *ret_bline = cur_bline;
            *ret_offset = boffset;
            break;
        }
        boffset -= cur_bline->char_count + 1; // Plus 1 for newline
        if (cur_bline->next) {
            cur_bline = cur_bline->next;
        } else {
            *ret_bline = cur_bline;
            *ret_offset = cur_bline->char_count;
            break;
        }
    }
    ATTO_RETURN_OK;
}

/** Get a bline by its line_index */
int buffer_get_bline(buffer_t* self, size_t line_index, bline_t** ret_bline) {
    // TODO optimize this
    bline_t* tmp_bline;
    for (tmp_bline = self->first_line; tmp_bline; tmp_bline = tmp_bline->next) {
        if (tmp_bline->line_index == line_index) {
            *ret_bline = tmp_bline;
            ATTO_RETURN_OK;
        }
    }
    ATTO_RETURN_ERR("line_index %lu does not exist", line_index);
}

/** TODO buffer_set_tab_stop */
int buffer_set_tab_stop(buffer_t* self, int tab_stop) {
    ATTO_RETURN_OK;
}

/** TODO buffer_get_tab_stop */
int buffer_get_tab_stop(buffer_t* self, int* ret_tab_stop) {
    ATTO_RETURN_OK;
}

/** Given a bline:offset or a boffset, return sanitized versions of both bline:offset and boffset */
int buffer_sanitize_position(buffer_t* self, bline_t* opt_bline, size_t opt_offset, size_t opt_boffset, bline_t** ret_bline, size_t* ret_offset, size_t* ret_boffset) {
    bline_t* tmp_bline;
    size_t tmp_offset;
    size_t tmp_boffset;
    if (opt_bline) {
        buffer_get_boffset(opt_bline, opt_offset, &tmp_boffset);
        buffer_get_bline_and_offset(self, tmp_boffset, &tmp_bline, &tmp_offset);
    } else {
        buffer_get_bline_and_offset(self, opt_boffset, &tmp_bline, &tmp_offset);
        buffer_get_boffset(tmp_bline, tmp_offset, &tmp_boffset);
    }
    if (ret_bline) *ret_bline = tmp_bline;
    if (ret_offset) *ret_offset = tmp_offset;
    if (ret_boffset) *ret_boffset = tmp_boffset;
    ATTO_RETURN_OK;
}

/** Deallocate a buffer */
int buffer_destroy(buffer_t* self) {
    ATTO_RETURN_OK;
}

// ============================================================================

/** Create a new bline */
static bline_t* bline_new(buffer_t* parent, char* data, size_t data_len, size_t line_index, baction_t* baction) {
    bline_t* self;

    self = calloc(1, sizeof(bline_t));
    self->buffer = parent;
    if (data_len < 1) {
        self->data = strdup("");
    } else {
        self->data = calloc(data_len, sizeof(char));
        memcpy(self->data, data, data_len);
    }
    self->data_stop = self->data + data_len;
    self->data_len = data_len;
    self->data_size = data_len;
    self->line_index = line_index;

    bline_update_char_count(self);
    bline_update_char_widths(self);

    if (baction) {
        (baction->delta).char_delta += self->char_count + 1;
        (baction->delta).byte_delta += self->data_len + 1;
        (baction->delta).line_delta += 1;
    }
    return self;
}

/** Update bline.char_count */
static void bline_update_char_count(bline_t* self) {
    char* c;
    self->char_count = 0;
    c = self->data;
    while (c < self->data_stop) {
        c += tb_utf8_char_length(*c);
        self->char_count += 1;
    }
}

/** Update bline.char_widths and bline.width */
static void bline_update_char_widths(bline_t* self) {
    char *c;
    int i;
    int w;
    if (self->char_count < 1) {
        return;
    }
    if (!self->char_widths) {
        self->char_widths = calloc(self->char_count, sizeof(int));
    } else {
        self->char_widths = realloc(self->char_widths, sizeof(int) * self->char_count);
    }
    self->width = 0;
    for (w = 0, i = 0, c = self->data; i < self->char_count && c < self->data_stop; i++) {
        if (*c == '\t') {
            w = self->buffer->tab_stop - (self->width % self->buffer->tab_stop);
        } else {
            w = 1;
        }
        self->char_widths[i] = w;
        self->width += w;
        c += tb_utf8_char_length(*c);
    }
}

/** Update buffer.filename and buffer.filemtime */
static void buffer_update_fstat(buffer_t* self, char* fname, FILE* fp) {
    struct stat fbuf;
    if (self->filename) {
        free(self->filename);
    }
    self->filename = fname;
    fstat(fileno(fp), &fbuf);
    self->filemtime = fbuf.st_mtime;
}

/** Invoked by buffer_insert */
static void buffer_insert_data(bline_t* self, size_t char_offset, char* data, size_t data_len, baction_t* baction, int do_fill_action) {
    int is_first;
    char* cur;
    char* data_stop;
    bline_t* cur_bline;
    bline_t* next_bline;
    char* after;
    size_t after_len;
    size_t num_newlines_added;
    size_t remaining_len;
    char* newline;
    size_t cur_len;
    buffer_t* buffer;

    is_first = 1;
    cur = data;
    data_stop = data + data_len;
    cur_bline = self;
    next_bline = self->next;
    after = NULL;
    after_len = 0;
    num_newlines_added = 0;
    buffer = self->buffer;

    // Copy data to action
    if (do_fill_action) {
        baction_append(baction, data, data_len);
    }

    while (1) {
        remaining_len = data_stop - cur;
        newline = memchr(cur, '\n', remaining_len);
        cur_len = (newline ? newline - cur : remaining_len);
        if (is_first && !newline) {
            // First bline, no more newlines
            // Just insert
            bline_insert(cur_bline, cur, cur_len, char_offset, baction);
        } else if (is_first && newline) {
            // First bline, more newlines to come
            if (char_offset == cur_bline->char_count) {
                // At EOL, so insert
                bline_insert(cur_bline, cur, cur_len, char_offset, baction);
            } else {
                // Not at EOL, replace after char_offset and remember replaced data
                bline_replace_after(cur_bline, cur, cur_len, char_offset, baction, &after, &after_len);
            }
        } else if (!is_first && newline) {
            // Not first bline, more newlines to come
            // Make new bline with content equal to cur:cur_len
            cur_bline = bline_new(cur_bline->buffer, cur, cur_len, cur_bline->line_index + 1, baction);
            bline_place(cur_bline, next_bline);
            num_newlines_added += 1;
        } else if (!is_first && !newline) {
            // Not first bline, no more newlines
            // Make new bline with content equal to cur:cur_len plus after data
            cur_bline = bline_new(cur_bline->buffer, cur, cur_len, cur_bline->line_index + 1, baction);
            bline_place(cur_bline, next_bline);
            if (after_len > 0) {
                bline_insert(cur_bline, after, after_len, cur_bline->char_count, baction);
            }
            num_newlines_added += 1;
        }
        if (!newline) {
            // No more newlines
            // Done!
            break;
        }

        cur = newline + 1; // Move cursor to 1 byte after the newline
        if (is_first) {
            is_first = 0; // No longer working on first bline
        }
    }

    // Update line_index of subsequent blines if we inserted any new blines
    if (num_newlines_added > 0) {
        buffer_update_line_indexes(buffer, next_bline, num_newlines_added);
    }

    // Update marks
    buffer_update_marks(buffer, baction);

    // Update buffer metadata
    buffer_update_metadata(buffer, baction);

    // Clean up
    if (after) free(after);
}

/** Invoked by buffer_delete */
static void buffer_delete_data(bline_t* self, size_t char_offset, size_t num_chars_to_delete, baction_t* baction, int do_fill_action) {
    size_t remaining_chars_to_delete;
    size_t num_chars_to_delete_on_line;
    size_t num_newlines_deleted;
    bline_t* end_bline;
    bline_t* target_bline;
    bline_t* tmp_bline;
    size_t end_offset;
    size_t copy_data_byte_offset;
    size_t copy_data_len;
    bline_t* del_start_bline;
    bline_t* del_stop_bline;
    buffer_t* buffer;

    remaining_chars_to_delete = num_chars_to_delete;
    num_newlines_deleted = 0;
    end_bline = self;
    end_offset = char_offset;
    buffer = self->buffer;

    // Find end_bline:end_offset on which the delete ends
    while (1) {
        num_chars_to_delete_on_line = end_bline->char_count - end_offset;
        if (end_bline->next && remaining_chars_to_delete > num_chars_to_delete_on_line) {
            remaining_chars_to_delete -= num_chars_to_delete_on_line + 1;
            end_bline = end_bline->next;
            end_offset = 0;
        } else {
            end_offset += remaining_chars_to_delete;
            break;
        }
    }

    // Copy data to baction
    if (do_fill_action) {
        baction_set_data_from_range(baction, self, char_offset, end_bline, end_offset);
    }

    // Delete data between self:char_offset and end_bline:end_offset
    if (end_bline == self) {
        // The delete takes place on a single bline
        bline_delete(self, char_offset, num_chars_to_delete, baction);
    } else {
        // The delete spans multiple lines!
        // First replace data after self:char_offset with data after end_bline:end_offset
        copy_data_byte_offset = bline_get_byte_from_char_offset(end_bline, end_offset);
        copy_data_len = (end_bline->data_len - copy_data_byte_offset); // Note, this can be zero
        bline_replace_after(self, end_bline->data + copy_data_byte_offset, copy_data_len, char_offset, baction, NULL, NULL);

        // Then remove blines between self->next and end_bline (inclusively)
        del_start_bline = self->next;
        del_stop_bline = end_bline->next;
        for (target_bline = del_start_bline; target_bline != del_stop_bline; ) {
            tmp_bline = target_bline->next;
            DL_DELETE(buffer->first_line, target_bline);
            bline_destroy(target_bline, baction);
            target_bline = tmp_bline;
            num_newlines_deleted += 1;
        }
    }

    // Update line_index of subsequent blines if we deleted any new blines
    if (num_newlines_deleted > 0) {
        buffer_update_line_indexes(buffer, self->next, num_newlines_deleted * -1);
    }

    // Update marks
    buffer_update_marks(buffer, baction);

    // Update buffer metadata
    buffer_update_metadata(buffer, baction);
}

/** Update marks in a buffer given edit deltas */
static void buffer_update_marks(buffer_t* self, baction_t* baction) {
    mark_node_t* mark_node;
    mark_node_t* tmp_mark_node;
    mark_t* mark;
    ssize_t char_delta;
    bline_t* orig_bline;
    char_delta = (baction->delta).char_delta;
    DL_FOREACH(self->mark_nodes, mark_node) {
        mark = mark_node->mark;
        if (mark->boffset < baction->boffset) {
            // Edit took place before mark, so skip
            continue;
        } else if (!mark->bline) {
            // The bline this mark was on got deleted. This means it now
            // belongs at baction bline:offset.
            mark->bline = baction->_unsafe_start_bline;
            mark->char_offset = baction->char_offset;
            mark->boffset = baction->boffset;
            DL_APPEND(baction->_unsafe_start_bline->mark_nodes, mark->_unsafe_parent_bline_mark_node);
            mark->_unsafe_parent_bline_mark_node = NULL;
        } else {
            if (baction->type == ATTO_BACTION_TYPE_INSERT) {
                mark_move(mark, char_delta);
                // Shift mark forward by char_delta
                //mark->boffset += char_delta;
            } else {
                // Shift mark backward by char_delta (which will be negative here)
                // but don't go past the edit boffset
                mark_set_boffset(mark, ATTO_MAX((ssize_t)mark->boffset + char_delta, (ssize_t)baction->boffset));
                //mark->boffset = ATTO_MAX((ssize_t)mark->boffset + char_delta, (ssize_t)baction->boffset);
            }
            // Update the bline and offset via the new boffset
            /*
            orig_bline = mark->bline;
            buffer_get_bline_and_offset(self, mark->boffset, &(mark->bline), &(mark->char_offset));
            mark->target_char_offset = mark->char_offset;
            // Relocate mark_node to new bline if needed
            if (orig_bline != mark->bline) {
                tmp_mark_node = NULL;
                DL_SEARCH_SCALAR(orig_bline->mark_nodes, tmp_mark_node, mark, mark);
                DL_DELETE(orig_bline->mark_nodes, tmp_mark_node);
                DL_APPEND(mark->bline->mark_nodes, tmp_mark_node);
            }
            */
        }
    }
}

/** Update buffer metadata given edit deltas */
static void buffer_update_metadata(buffer_t* self, baction_t* baction) {
    self->char_count += (baction->delta).char_delta;
    self->byte_count += (baction->delta).byte_delta;
    if ((baction->delta).line_delta != 0) {
        self->line_count += (baction->delta).line_delta;
        self->line_digits = (int)log10((double)self->line_count) + 1;
    }
    self->has_unsaved_changes = 1;
}

/** Adjust line_indexes starting from start by line_delta */
static void buffer_update_line_indexes(buffer_t* buffer, bline_t* start, ssize_t line_delta) {
    bline_t* tmp_bline;
    for (tmp_bline = start; tmp_bline; tmp_bline = tmp_bline->next) {
        tmp_bline->line_index += line_delta;
    }
    buffer->last_line = buffer->first_line->prev; // utlist macros ensure that head.prev==tail
}

/** Insert data into a bline at char_offset */
static void bline_insert(bline_t* self, char* data, size_t data_len, size_t char_offset, baction_t* baction) {
    size_t new_data_len;
    size_t byte_offset;
    size_t orig_char_count;

    if (data_len < 1) {
        return;
    }

    // Find byte_offset from char_offset
    byte_offset = bline_get_byte_from_char_offset(self, char_offset);

    // Ensure room for new data
    new_data_len = self->data_len + data_len;
    if (self->data_size < new_data_len) {
        self->data = realloc(self->data, sizeof(char) * new_data_len);
        self->data_size = new_data_len;
    }

    // Shift existing data if necessary
    if (char_offset < self->char_count) {
        memmove(self->data + byte_offset + data_len, self->data + byte_offset, self->data_len - byte_offset);
    }

    // Insert data
    memcpy(self->data + byte_offset, data, data_len);

    // Update metadata
    (baction->delta).byte_delta += (new_data_len - self->data_len); // byte_delta
    self->data_len = new_data_len;
    self->data_stop = self->data + self->data_len;
    orig_char_count = self->char_count;
    bline_update_char_count(self);
    bline_update_char_widths(self);
    (baction->delta).char_delta += (self->char_count - orig_char_count); // char_delta
}

/** Delete num_chars_to_delete from bline->data beginning at char_offset */
static void bline_delete(bline_t* self, size_t char_offset, size_t num_chars_to_delete, baction_t* baction) {
    size_t start_byte_offset;
    size_t end_byte_offset;
    size_t num_bytes_to_delete;
    size_t orig_char_count;

    if (num_chars_to_delete < 1) {
        return;
    }

    // Find byte_offsets
    start_byte_offset = bline_get_byte_from_char_offset(self, char_offset);
    end_byte_offset = bline_get_byte_from_char_offset(self, char_offset + num_chars_to_delete);
    num_bytes_to_delete = (end_byte_offset - start_byte_offset);

    // Shift data down
    memmove(self->data + start_byte_offset, self->data + end_byte_offset, self->data_len - end_byte_offset);

    // Update metadata
    (baction->delta).byte_delta -= num_bytes_to_delete; // byte_delta
    self->data_len -= num_bytes_to_delete;
    self->data_stop = self->data + self->data_len;
    orig_char_count = self->char_count;
    bline_update_char_count(self);
    bline_update_char_widths(self);
    (baction->delta).char_delta += (self->char_count - orig_char_count); // char_delta
}

/** Replace bline->data after char_offset with data and return replaced data */
static void bline_replace_after(bline_t* self, char* data, size_t data_len, size_t char_offset, baction_t* baction, char **ret_after, size_t* ret_after_len) {
    size_t new_data_len;
    size_t byte_offset;
    size_t orig_char_count;

    // Find byte_offset from char_offset
    byte_offset = bline_get_byte_from_char_offset(self, char_offset);

    // Ensure room for new data
    new_data_len = byte_offset + data_len;
    if (self->data_size < new_data_len) {
        self->data = realloc(self->data, sizeof(char) * new_data_len);
        self->data_size = new_data_len;
    }

    // Copy replaced data if requested
    if (ret_after && ret_after_len) {
        *ret_after_len = self->data_len - byte_offset;
        if (*ret_after_len > 0) {
            *ret_after = malloc(sizeof(char) * (*ret_after_len));
            memcpy(*ret_after, self->data + byte_offset, *ret_after_len);
        } else {
            *ret_after = NULL;
        }
    }

    // Insert data
    if (data_len > 0) {
        memcpy(self->data + byte_offset, data, data_len);
    }

    // Update metadata
    (baction->delta).byte_delta += (new_data_len - self->data_len); // byte_delta
    self->data_len = new_data_len;
    self->data_stop = self->data + self->data_len;
    orig_char_count = self->char_count;
    bline_update_char_count(self);
    bline_update_char_widths(self);
    (baction->delta).char_delta += (self->char_count - orig_char_count); // char_delta
}

/** Place a bline in the linked list */
static void bline_place(bline_t* self, bline_t* before) {
    if (before) {
        DL_PREPEND_ELEM(self->buffer->first_line, before, self);
    } else {
        DL_APPEND(self->buffer->first_line, self);
    }
}

/** Deallocate a bline */
static void bline_destroy(bline_t* self, baction_t* baction) {
    mark_node_t* mark_node;
	if (baction) {
		(baction->delta).char_delta -= self->char_count + 1;
		(baction->delta).byte_delta -= self->data_len + 1;
		(baction->delta).line_delta -= 1;
	}
    if (self->data) free(self->data);
    if (self->char_styles) free(self->char_styles);
    if (self->char_widths) free(self->char_widths);
    if (self->mark_nodes) {
        DL_FOREACH(self->mark_nodes, mark_node) {
            mark_node->mark->bline = NULL;
            mark_node->mark->_unsafe_parent_bline_mark_node = mark_node;
        }
    }
    free(self);
}


/** Get a byte offset given a char offset in a bline */
static size_t bline_get_byte_from_char_offset(bline_t* self, size_t char_offset) {
    char* c;
    size_t char_count;
    c = self->data;
    char_count = 0;
    while (char_count < char_offset && c < self->data_stop) {
        c += tb_utf8_char_length(*c);
        char_count += 1;
    }
    return (size_t)(c - self->data);
}

/** Create a new baction */
static baction_t* baction_new(bline_t* bline, size_t char_offset, size_t boffset, int type) {
    baction_t* self;
    self = calloc(1, sizeof(baction_t));
    self->line_index = bline->line_index;
    self->char_offset = char_offset;
    self->boffset = boffset;
    self->type = type;
    (self->delta).char_delta = 0;
    (self->delta).byte_delta = 0;
    (self->delta).line_delta = 0;
    self->buffer = bline->buffer;
    self->_unsafe_start_bline = bline;
    return self;
}

/** Set baction->data from a range in the buffer */
static void baction_set_data_from_range(baction_t* self, bline_t* start_bline, size_t start_offset, bline_t* end_bline, size_t end_offset) {
    bline_t* tmp_bline;
    size_t start_byte_offset;
    size_t end_byte_offset;

    start_byte_offset = bline_get_byte_from_char_offset(start_bline, start_offset);
    end_byte_offset = bline_get_byte_from_char_offset(end_bline, end_offset);

    if (start_bline == end_bline) {
        // Copy data from a single line
        baction_append(self, start_bline->data + start_byte_offset, end_byte_offset - start_byte_offset);
    } else {
        for (tmp_bline = start_bline; tmp_bline && tmp_bline != end_bline->next; tmp_bline = tmp_bline->next) {
            if (tmp_bline == start_bline) {
                if (start_offset < start_bline->char_count) {
                    // Copy data from start_line
                    baction_append(self, start_bline->data + start_byte_offset, start_bline->data_len - start_byte_offset);
                }
                baction_append(self, (char*)"\n", 1); // Append a newline
            } else if (tmp_bline == end_bline) {
                if (end_offset > 0) {
                    // Copy data from end_line
                    baction_append(self, end_bline->data, end_byte_offset);
                }
            } else {
                // Copy data from middle line and append a newline
                baction_append(self, tmp_bline->data, tmp_bline->data_len);
                baction_append(self, (char*)"\n", 1);
            }
        }
    }
}


/** Append data to a baction */
static void baction_append(baction_t* self, char* data, size_t data_len) {
    if (!self->data) {
        self->data_len = data_len;
        self->data = malloc(sizeof(char) * data_len);
        memcpy(self->data, data, data_len);
    } else {
        self->data = realloc(self->data, sizeof(char) * (self->data_len + data_len));
        memcpy(self->data + self->data_len, data, data_len);
        self->data_len += data_len;
    }
}

/** Re-perform baction, optionally overriding position */
static int baction_do(baction_t* self, bline_t* override_bline, size_t override_char_offset) {
    int rc;
    bline_t* bline;
    size_t char_offset;

    // Handle override
    if (override_bline) {
        bline = override_bline;
        char_offset = override_char_offset;
    } else {
        bline = NULL;
        buffer_get_bline(self->buffer, self->line_index, &bline);
        char_offset = self->char_offset;
    }

    // Error check
    if (!bline) {
        ATTO_RETURN_ERR("Failed to find bline with line_index=%lu", self->line_index);
    } else if (char_offset > bline->char_count) {
        ATTO_RETURN_ERR("Action char_offset=%lu does not exist in bline with char_count=%lu", char_offset, bline->char_count);
    }

    // Perform action
    if (self->type == ATTO_BACTION_TYPE_INSERT) {
        rc = buffer_insert(bline, char_offset, self->data, self->data_len, 0);
    } else {
        rc = buffer_delete(bline, char_offset, (size_t)((self->delta).char_delta * -1), 0);
    }
    return rc;
}

/** Undo a baction */
static int baction_undo(baction_t* self) {
    int rc;
    bline_t* bline;

    // Error check
    bline = NULL;
    buffer_get_bline(self->buffer, self->line_index, &bline);
    if (!bline) {
        ATTO_RETURN_ERR("Failed to find bline with line_index=%lu", self->line_index);
    } else if (self->char_offset > bline->char_count) {
        ATTO_RETURN_ERR("Action char_offset=%lu does not exist in bline with char_count=%lu", self->char_offset, bline->char_count);
    }

    // Perform action
    if (self->type == ATTO_BACTION_TYPE_INSERT) {
        rc = buffer_delete(bline, self->char_offset, (self->delta).char_delta, 0);
    } else {
        rc = buffer_insert(bline, self->char_offset, self->data, self->data_len, 0);
    }
    return rc;
}

/** Record a baction in the buffer's action list */
static void buffer_record_action(buffer_t* self, baction_t* baction) {
    baction_t* baction_target;
    baction_t* baction_tmp;
    int do_delete;

    if (self->action_undone) {
        // We are recording an action after an undo has been performed, so we
        // need to chop off the tail of the baction list before recording the
        // new one.
        // TODO could implement multilevel undo here instead
        do_delete = 0;
        DL_FOREACH_SAFE(self->actions, baction_target, baction_tmp) {
            if (!do_delete && baction_target == self->action_undone) {
                do_delete = 1;
            }
            if (do_delete) {
                DL_DELETE(self->actions, baction_target);
                baction_destroy(baction_target);
            }
        }
    }

    // Append baction to the list
    DL_APPEND(self->actions, baction);
    self->action_tail = baction;
    self->action_undone = NULL;
}

/** Deallocate a baction */
static void baction_destroy(baction_t* self) {
    if (self->data) free(self->data);
    free(self);
}

/** Test harness */
int mainx(int argc, char** argv) {
    buffer_t* buf;
    mark_t* mark;
    bline_t* bline;
    size_t offset;

    buffer_new(&buf);

    mark_new(buf->first_line, 0, "", 0, NULL, &mark);
    buffer_print(buf);

    printf("insert\n");
    buffer_insert(buf->first_line, 0, "adam\n==\ndead\n\nwhale", 15 + 4, 1); // insert
    mark_new(buf->first_line->next, 1, "", 0, NULL, &mark);
    buffer_print(buf);

    printf("delete\n");
    buffer_delete(buf->first_line, 3, 8, 1); // delete
    buffer_print(buf);

    printf("undo delete\n");
    buffer_undo(buf); // undo delete
    buffer_print(buf);

    printf("undo insert\n");
    buffer_undo(buf); // undo insert
    buffer_print(buf);

    printf("redo insert\n");
    buffer_redo(buf); // redo insert
    buffer_print(buf);

    printf("redo delete\n");
    buffer_redo(buf); // redo delete
    buffer_print(buf);

    printf("undo redo delete\n");
    buffer_undo(buf); // undo delete
    buffer_print(buf);

    printf("undo redo insert\n");
    buffer_undo(buf); // undo insert
    buffer_print(buf);

    return 0;
}

/** Print buffer to stdout */
void buffer_print(buffer_t* self) {
    char* markbuf;
    size_t markbuf_len;
    bline_t* bline;
    mark_node_t* mark_node;

    markbuf_len = 1024;
    markbuf = malloc(markbuf_len + 1);
    for (bline = self->first_line; bline; bline = bline->next) {
        printf("%3lu '%.*s' b=%lu c=%lu %p\n", bline->line_index, (int)bline->data_len, bline->data, bline->data_len, bline->char_count, bline);
        if (bline->char_count > markbuf_len) {
            markbuf_len = bline->char_count;
            markbuf = realloc(markbuf, markbuf_len + 1);
        }
        snprintf(markbuf, bline->char_count + 1, "%*.*s", (int)bline->char_count, (int)bline->char_count, " ");
        DL_FOREACH(bline->mark_nodes, mark_node) {
            markbuf[mark_node->mark->char_offset] = '^';
        }
        printf("     %s", markbuf);
        DL_FOREACH(bline->mark_nodes, mark_node) {
            printf(" %p", mark_node);
        }
        printf("\n");

    }
    printf("\n");
    free(markbuf);
}
