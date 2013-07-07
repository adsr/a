#include "atto.h"

/**
 * Allocate a new buffer
 */
buffer_t* buffer_new() {
    buffer_t* buffer;
    buffer = (buffer_t*)calloc(1, sizeof(buffer_t));
    buffer->line_count = 1;
    buffer->data = (char*)calloc(ATTO_BUFFER_DATA_ALLOC_INCR, sizeof(char));
    buffer->data_size = ATTO_BUFFER_DATA_ALLOC_INCR;
    buffer->blines = (bline_t*)calloc(ATTO_LINE_OFFSET_ALLOC_INCR, sizeof(bline_t));
    buffer->blines_size = ATTO_LINE_OFFSET_ALLOC_INCR;
    // TODO check calloc retvals
    // TODO prev/next buffer
    return buffer;
}

/**
 * Read contents of filename into buffer
 */
int buffer_read(buffer_t* self, char* filename) {
    FILE* f;
    int filesize;
    int fd;
    struct stat filestat;
    char* data;

    // Open file
    f = fopen(filename, "rb");
    if (!f) {
        ATTO_DEBUG_PRINTF("Could not open file for reading: %s\n", filename);
        return ATTO_RC_ERR;
    }

    // Stat file
    fd = fileno(f);
    fstat(fd, &filestat);

    // Ask for position
    filesize = filestat.st_size;

    // Allocate buffer data
    data = (char*)malloc(filesize);
    if (!data) {
        ATTO_DEBUG_PRINTF("Could not allocate %d bytes for file %s\n", filesize, filename);
        fclose(f);
        return ATTO_RC_ERR;
    }

    // Read file into buffer data
    fread(data, filesize, 1, f);

    // Close file
    fclose(f);

    // Set buffer data
    buffer_set(self, data, filesize);

    // Update filename and mtime
    self->filename = strdup(filename);
    self->filemtime = filestat.st_mtime;
    self->has_unsaved_changes = 0;

    return ATTO_RC_OK;
}

/**
 * Write contents of buffer to filename
 */
int buffer_write(buffer_t* self, char* filename, int is_append) {
    FILE* f;

    // Open file
    f = fopen(filename, is_append ? "a" : "w");
    if (!f) {
        ATTO_DEBUG_PRINTF("Could not open file for writing: %s\n", filename);
        return ATTO_RC_ERR;
    }

    // Write to file
    fwrite(self->data, self->byte_count, 1, f);

    // Close file
    fclose(f);

    // Update filename and mtime
    self->filename = strdup(filename);
    self->filemtime = time(NULL);
    self->has_unsaved_changes = 0;

    return ATTO_RC_OK;
}

/**
 * Clear contents of buffer
 */
int buffer_clear(buffer_t* self) {
    return buffer_delete(self, 0, self->byte_count);
}

/**
 * Set contents of buffer to data
 */
int buffer_set(buffer_t* self, char* data, int len) {
    buffer_clear(self);
    buffer_insert(self, 0, data, len, NULL, NULL, NULL);
    return ATTO_RC_OK;
}

/**
 * Insert content in a buffer
 */
int buffer_insert(buffer_t* self, int offset, char* str, int len, int* ret_offset, int* ret_line, int* ret_col) {
    int line;
    int col;

    ATTO_DEBUG_PRINTF("[%s]:%d at offset=%d\n", str, len, offset);

    // Sanitize input
    offset = ATTO_MAX(0, ATTO_MIN(offset, self->byte_count));
    buffer_get_line_col(self, offset, &line, &col);

    // Nothing to do if len is lt 1
    if (len < 1) {
        return ATTO_RC_OK;
    }

    // Expand buffer data if needed
    if (self->byte_count + len > self->data_size) {
        self->data_size = self->byte_count + ATTO_MAX(len, ATTO_BUFFER_DATA_ALLOC_INCR);
        self->data = (char*)realloc(self->data, sizeof(char) * self->data_size);
    }

    // Shift data to the right to make space for new content
    memmove(
        self->data + offset + len,
        self->data + offset,
        self->byte_count - offset
    );

    // Copy new content to buffer data
    memcpy(
        self->data + offset,
        str,
        len
    );

    // Update byte_count
    self->byte_count += len;

    // Mark unsaved changes
    if (!self->has_unsaved_changes) {
        self->has_unsaved_changes = 1;
    }

    // Update metadata
    _buffer_update_metadata(self, offset, line, col, str, len);

    // Return values
    if (ret_offset) {
        *ret_offset = offset + len;
    }
    if (ret_line || ret_col) {
        buffer_get_line_col(self, offset + len, ret_line, ret_col);
    }

    // TODO invoke listeners
    return ATTO_RC_OK;
}

/**
 * Delete content from a buffer
 */
int buffer_delete(buffer_t* self, int offset, int len) {
    int end_offset;
    int line;
    int col;
    char* delta;

    ATTO_DEBUG_PRINTF("%d at offset=%d\n", len, offset);

    // Nothing to do if byte_count is lt 1
    if (self->byte_count < 1) {
        return ATTO_RC_OK;
    }

    // Sanitize input
    offset = ATTO_MAX(0, ATTO_MIN(offset, self->byte_count - 1));
    buffer_get_line_col(self, offset, &line, &col);
    end_offset = offset + len;
    end_offset = ATTO_MAX(0, ATTO_MIN(end_offset, self->byte_count));
    len = end_offset - offset;

    // Nothing to do if len is lt 1
    if (len < 1) {
        return ATTO_RC_OK;
    }

    // Copy deleted content to delta
    delta = (char*)malloc(sizeof(char) * (len + 1));
    delta[len] = '\0';
    memcpy(
        delta,
        self->data + offset,
        len
    );

    // Shift data to the left
    memmove(
        self->data + offset,
        self->data + end_offset,
        self->byte_count - end_offset
    );

    // Update byte_count
    self->byte_count -= len;

    // Mark unsaved changes
    if (!self->has_unsaved_changes) {
        self->has_unsaved_changes = 1;
    }

    // Update metadata
    _buffer_update_metadata(self, offset, line, col, delta, len * -1);

    // TODO invoke listeners
    return ATTO_RC_OK;
}

/**
 * Get the offset and length of a line
 */
int buffer_get_line_offset_len(buffer_t* self, int line, int from_col, int* ret_offset, int* ret_len) {
    int line_offset;
    int line_len;

    // Calc line offset and length
    line_offset = buffer_get_offset(self, line, from_col);
    buffer_get_line_col(self, line_offset, &line, &from_col);

    if (line == self->line_count - 1) {
        line_len = (self->byte_count - self->blines[self->line_count - 1].offset) - from_col;
    } else {
        line_len = (buffer_get_offset(self, line + 1, 0) - 1) - line_offset;
        line_len = ATTO_MAX(line_len, 0);
    }

    if (ret_offset) {
        *ret_offset = line_offset;
    }
    if (ret_len) {
        *ret_len = line_len;
    }
}

/**
 * Get a single line from a buffer
 */
int buffer_get_line(buffer_t* self, int line, int from_col, char* usebuf, int usebuf_len, char** ret_line, int* ret_len) {
    int line_offset;
    int line_len;
    buffer_get_line_offset_len(self, line, from_col, &line_offset, &line_len);
    return buffer_get_substr(self, line_offset, line_len, usebuf, usebuf_len, ret_line, ret_len);
}

int buffer_get_substr(buffer_t* self, int offset, int len, char* usebuf, int usebuf_len, char** ret_substr, int* ret_len) {
    // Sanitize inputs
    offset = ATTO_MIN(ATTO_MAX(offset, 0), self->byte_count);
    if (len < 0) { // Shortcut for returning til end of buffer
        len = self->byte_count;
    }
    len = ATTO_MIN(ATTO_MAX(len, 0), self->byte_count - offset);

    // Allocate usebuf if not provided
    if (!usebuf || usebuf_len < 0) {
        usebuf = (char*)malloc(sizeof(char) * (len + 1));
        usebuf_len = len;
    } else {
        usebuf_len = ATTO_MIN(len, usebuf_len);
    }

    // Copy contents to usebuf (not exceeding usebuf_len bytes)
    memcpy(usebuf, self->data + offset, usebuf_len);
    usebuf[usebuf_len] = '\0';

    // Set return values
    if (ret_substr) {
        *ret_substr = usebuf;
    }
    if (ret_len) {
        *ret_len = len;
    }

    return ATTO_RC_OK;
}

/**
 * Given an offset, return a line and column
 */
int buffer_get_line_col(buffer_t* self, int offset, int* ret_line, int* ret_col) {
    int line;
    int prev_offset;

    // Clamp offset
    offset = ATTO_MAX(ATTO_MIN(offset, self->byte_count), 0);

    if (offset == 0) {
        // Handle case for offset zero
        if (ret_line) *ret_line = 0;
        if (ret_col) *ret_col = 0;
        return ATTO_RC_OK;
    }

    // Find first line offset greater than offset
    // TODO can optimize this with a binary search
    prev_offset = 0;
    for (line = 0; line < self->line_count; line++) {
        if (self->blines[line].offset == offset) {
            if (ret_line) *ret_line = line;
            if (ret_col) *ret_col = 0;
            return ATTO_RC_OK;
        } else if (self->blines[line].offset > offset) {
            if (ret_line) *ret_line = ATTO_MAX(line - 1, 0);
            if (ret_col) *ret_col = ATTO_MAX(offset - prev_offset, 0);
            return ATTO_RC_OK;
        }
        prev_offset = self->blines[line].offset;
    }

    // If we get here, it must be on the last line
    if (ret_line) *ret_line = ATTO_MAX(self->line_count - 1, 0);
    if (ret_col) *ret_col = ATTO_MIN(ATTO_MAX(offset - prev_offset, 0), self->byte_count - prev_offset);
    return ATTO_RC_OK;
}

/**
 * Given a line and column, return an offset
 */
int buffer_get_offset(buffer_t* self, int line, int col) {
    int offset;

    // Sanitize input
    line = ATTO_MAX(ATTO_MIN(line, self->line_count - 1), 0);
    col = ATTO_MAX(col, 0);

    // Get offset for line
    offset = self->blines[line].offset;

    // Make sure col is not past end of line
    if (line == self->line_count - 1) {
        col = ATTO_MIN(col, self->byte_count - offset);
    } else {
        col = ATTO_MIN(col, (self->blines[line + 1].offset - 1) - offset);
    }

    return offset + col;
}

/**
 * Return the offset of the first occurence of needle after offset
 * Return -1 if not found
 */
int buffer_search(buffer_t* self, char* needle, int offset) {
    return _buffer_search(self, needle, offset, 0);
}

/**
 * Return the offset of the first occurence of needle before offset
 * Return -1 if not found
 */
int buffer_search_reverse(buffer_t* self, char* needle, int offset) {
    return _buffer_search(self, needle, offset, 1);
}

/**
 * Return the offset of the first occurence of regex after offset
 * Return -1 if not found or if regex is invalid
 */
int buffer_regex(buffer_t* self, char* regex, int offset, int length) {
    pcre* re;
    int rc;
    int results[3];
    re = util_compile_regex(regex);
    if (!re) {
        return -1;
    }
    offset = ATTO_MIN(ATTO_MAX(offset, 0), self->byte_count - 1);
    if (length < 0) {
        length = self->byte_count - offset;
    } else {
        length = ATTO_MIN(ATTO_MAX(length, 0), self->byte_count - offset);
    }
    rc = pcre_exec(re, NULL, self->data + offset, length, 0, 0, results, 3);
    pcre_free(re);
    if (rc < 0) {
        return -1;
    }
    return offset + results[0];
}

/**
 * Search for needle in buffer before/after offset
 * Invoked by buffer_search and buffer_search_reverse
 */
int _buffer_search(buffer_t* self, char* needle, int offset, int is_reverse) {
    int needle_len;
    int scan_len;
    char* match;
    int match_offset;

    // Clamp offset
    offset = ATTO_MAX(ATTO_MIN(offset, self->byte_count - 1), 0);

    // Calc scan length
    scan_len = is_reverse ? offset : (self->byte_count - offset);

    // Find match
    needle_len = strlen(needle);
    if (needle_len == 1) {
        if (is_reverse) {
            match = (char*)memrchr(self->data + offset, *needle, scan_len);
        } else {
            match = (char*)memchr(self->data + offset, *needle, scan_len);
        }
    } else {
        if (is_reverse) {
            match = (char*)util_memrmem(self->data + offset, scan_len, needle, needle_len);
        } else {
            match = (char*)memmem(self->data + offset, scan_len, needle, needle_len);
        }
    }

    if (!match) {
        // Not found
        return -1;
    }

    // Return match offset
    return match - self->data;
}

/**
 * Add a style
 */
int buffer_add_style(buffer_t* self, srule_t* rule) {
    srule_node_t* node;
    node = (srule_node_t*)calloc(1, sizeof(srule_node_t));
    node->rule = rule;
    LL_APPEND(self->styles, node);
    return ATTO_RC_OK;
}

/**
 * Remove a style
 */
int buffer_remove_style(buffer_t* self, srule_t* rule) {
    srule_node_t* elt;
    srule_node_t* tmp;
    LL_FOREACH_SAFE(self->styles, elt, tmp) {
        if (elt->rule == rule) {
            LL_DELETE(self->styles, elt);
        }
    }
    return ATTO_RC_OK;
}

/**
 * Add a mark
 */
mark_t* buffer_add_mark(buffer_t* self, int offset) {
    mark_t* mark;
    int line;
    int col;
    buffer_get_line_col(self, offset, &line, &col);
    offset = buffer_get_offset(self, line, col);
    mark = (mark_t*)calloc(1, sizeof(mark_t));
    mark->buffer = self;
    mark->line = line;
    mark->col = col;
    mark->target_col = col;
    mark->offset = offset;
    LL_APPEND(self->marks, mark);
    return mark;
}

/**
 * Remove a mark
 */
int buffer_remove_mark(buffer_t* self, mark_t* mark) {
    LL_DELETE(self->marks, mark);
    free(mark);
    return ATTO_RC_OK;
}

/**
 * Add a buffer listener
 */
blistener_t* buffer_add_buffer_listener(buffer_t* self, void* listener, blistener_callback_t fn) {
    blistener_t* blistener;
    blistener = (blistener_t*)calloc(1, sizeof(blistener_t));
    blistener->listener = listener;
    blistener->fn = fn;
    LL_APPEND(self->listeners, blistener);
    return ATTO_RC_OK;
}

/**
 * Remove a buffer listener
 */
int buffer_remove_buffer_listener(buffer_t* self, blistener_t* blistener) {
    LL_DELETE(self->listeners, blistener);
    free(blistener);
    return ATTO_RC_OK;
}

/**
 * Update various metadata of a buffer after it has been edited
 */
int _buffer_update_metadata(buffer_t* self, int offset, int line, int col, char* delta, int delta_len) {
    _buffer_update_blines(self, offset, line, col, delta, delta_len);
    _buffer_update_marks(self, offset, delta_len);
    _buffer_update_styles(self, line, delta, delta_len, 1);
    _buffer_notify_listeners(self, line, col, delta, delta_len);

/*
char* all;
buffer_get_substr(self, 0, -1, NULL, 0, &all, NULL);
ATTO_DEBUG_PRINTF("buffer_data=[%s]\n", all);
free(all);
*/

    return ATTO_RC_OK;
}

/**
 * Notify listeners of an edit to the buffer
 */
int _buffer_notify_listeners(buffer_t* self, int line, int col, char* delta, int delta_len) {
    blistener_t* listener;
    LL_FOREACH(self->listeners, listener) {
        (listener->fn)(self, listener->listener, line, col, delta, delta_len);
    }
    return ATTO_RC_OK;
}

/**
 * Update blines array
 */
int _buffer_update_blines(buffer_t* self, int dirty_offset, int dirty_line, int dirty_col, char* delta, int delta_len) {
    int newline_delta;
    int line;
    int line_stop;
    int orig_line_count;
    int shift_src;
    int shift_dest;
    int shift_size;
    int offset;
    int prev_offset;
    char* newline;
    sspan_t* spans;

    // Sanitize input
    dirty_line = ATTO_MIN(self->line_count - 1, ATTO_MAX(0, dirty_line)); // TODO not sure this is needed

    // Count newlines in delta
    newline_delta = util_memchr_count('\n', delta, abs(delta_len)) * (delta_len > 0 ? 1 : -1);
ATTO_DEBUG_PRINTF("newline_delta=%d\n", newline_delta);

    // If there are no newlines in delta...
    if (newline_delta == 0) {
        // 1. Update length of dirty_line
        self->blines[dirty_line].length += delta_len;

        // 2. Update offsets of lines after dirty_line
        for (line = dirty_line + 1; line < self->line_count; line++) {
            self->blines[line].offset += delta_len;
        }

        // Done!
goto winners;
        //return ATTO_RC_OK;
    }

    // If we get here, it means newlines are present in the delta

    // 1. Update line_count
    orig_line_count = self->line_count;
    self->line_count += newline_delta;
    if (self->line_count > self->blines_size) {
        // Need to expand blines array
        self->blines_size = self->line_count;
        self->blines = (bline_t*)realloc(
            self->blines,
            sizeof(bline_t) * self->blines_size
        );
    }
ATTO_DEBUG_PRINTF("orig_line_count=%d new_line_count=%d\n", orig_line_count, self->line_count);

    // 2. Shift blines up or down
    offset = self->blines[dirty_line].offset; // Save orig offset
    shift_dest = newline_delta > 0 ? (dirty_line + 1 + newline_delta) : (dirty_line);
    shift_src = newline_delta > 0 ? (dirty_line + 1) : (dirty_line - newline_delta);
    shift_size = newline_delta > 0 ? (orig_line_count - dirty_line - 1) : (orig_line_count - dirty_line + newline_delta);
    if (shift_size > 0) {
        memmove(
            self->blines + shift_dest,
            self->blines + shift_src,
            sizeof(bline_t) * shift_size
        );
ATTO_DEBUG_PRINTF("shifted bline block of size %d @ %d to %d\n", shift_size, shift_src, shift_dest);
    }

    // 3. Alloc sspans for new lines
    if (newline_delta > 0) {
        line = dirty_line + 1;
        line_stop = line + newline_delta + 1;
    } else {
        line = orig_line_count;
        line_stop = self->blines_size;
    }
ATTO_DEBUG_PRINTF("resetting spans from=%d until=%d\n", line, line_stop);
    for (; line < line_stop; line++) {
        self->blines[line].sspans_size = ATTO_SSPAN_RANGE_ALLOC_INCR;
        self->blines[line].sspans = (sspan_t*)calloc(ATTO_SSPAN_RANGE_ALLOC_INCR, sizeof(sspan_t));
        self->blines[line].sspans_len = 0;
    }

    // 4. Invalidate styles
    if (dirty_line + 1 < self->line_count) {
        self->blines[dirty_line + 1].sspans_len = 0;
    }
    if (newline_delta > 0) {
        for (line = dirty_line + 2; line < dirty_line + newline_delta; line++) {
            self->blines[line].sspans_len = 0;
        }
    }

    // 5. Recalculate offsets and lengths
    line = dirty_line;
    self->blines[line].offset = offset;
    while (offset < self->byte_count) {
        // Look for a newline
        newline = (char*)memchr(self->data + offset, '\n', self->byte_count - offset);
        if (!newline) {
            // No more newlines
            break;
        }

        // Found a newline!
        line += 1;

        // Calculate line offset (1 byte after the newline we just found)
        prev_offset = offset;
        offset = (newline + 1) - self->data;

        // Set length of previous line
        self->blines[line - 1].length = (offset - prev_offset) - 1;

        // Set offset of this line
        self->blines[line].offset = offset;

        // Remember offset
        prev_offset = offset;
    }

    // Update line_count
    self->line_count = line + 1;

    // Set length of last line
    self->blines[line].length = self->byte_count - self->blines[line].offset;

winners: for (line = 0; line < self->line_count; line++) {
ATTO_DEBUG_PRINTF("bline[%d] o=%d l=%d sspans_len=%d\n", line, self->blines[line].offset, self->blines[line].length, self->blines[line].sspans_len);
}

    return ATTO_RC_OK;
}

/**
 * Update marks after offset by delta
 */
int _buffer_update_marks(buffer_t* self, int offset, int delta) {
    mark_t* mark;
    int line;
    int col;

    // Update all marks gte offset
    LL_FOREACH(self->marks, mark) {
        if (mark->offset >= offset) {
            mark_move(mark, delta);
        }
    }

    return ATTO_RC_OK;
}

/**
 * Apply style rules to lines after dirty_line
 */
int _buffer_update_styles(buffer_t* self, int dirty_line, char* delta, int delta_len, int bail_on_matching_style) {
    int line;
    bline_t* bline;
    srule_node_t* node;
    int* char_attrs;
    int char_attrs_size;
    srule_t* open_rule;
    srule_t* rule;
    int matches[3];
    int style_from_col;
    int col;
    int re_offset;
    int multi_start;
    mark_t* range_start;
    mark_t* range_end;
    int attr_col;
    int rc;
    int line_style_changed;
    int sspans_len;

    // Update styles starting from dirty_line
    char_attrs_size = 0;
    char_attrs = NULL;
    open_rule = dirty_line > 0 ? self->blines[dirty_line - 1].open_rule : NULL;

ATTO_DEBUG_PRINTF("line[%d].open_rule = %p\n", dirty_line - 1, open_rule);
    for (line = dirty_line; line < self->line_count; line++) {
        bline = (self->blines + line);
        style_from_col = 0;

        // Nothing to style if length is zero
        if (bline->length < 1) {
            if (open_rule && !bline->open_rule) {
                bline->open_rule = open_rule;
            }
            goto reduce_char_attrs;
        }

        // Expand char_attrs if needed
        if (char_attrs_size < bline->length) {
ATTO_DEBUG_PRINTF("%s\n", "resized char_attrs");
            char_attrs_size = bline->length;
            if (char_attrs) {
                char_attrs = (int*)realloc(char_attrs, char_attrs_size * sizeof(int));
            } else {
                char_attrs = (int*)malloc(char_attrs_size * sizeof(int));
            }
        }
        memset(char_attrs, 0, sizeof(int) * char_attrs_size);

        // If there's an open rule, see if it ends on this line
        if (open_rule) {
ATTO_DEBUG_PRINTF("%s\n", "open_rule!");
            // See if open_rule ends on this line
            if (open_rule->type == ATTO_SRULE_TYPE_MULTI) {
                // See if cregex_end is on this line
                if (pcre_exec(open_rule->cregex_end, NULL, self->data + bline->offset, bline->length, 0, 0, matches, 3) >= 0) {
                    // Match!
                    // matches[1] will be set by pcre_exec
                } else {
                    // No match
                    matches[1] = -1;
                }
            } else if (open_rule->type == ATTO_SRULE_TYPE_RANGE) {
                // See if range_end is on this line
                range_end = open_rule->range_start->offset < open_rule->range_end->offset ? open_rule->range_end : open_rule->range_start;
                if (range_end->offset >= bline->offset
                    && range_end->offset < bline->offset + bline->length
                ) {
                    // Match!
                    matches[1] = range_end->col;
                } else {
                    // No match
                    matches[1] = -1;
                }
            }

            // If we have a match:
            //    This means open_rule ends on this line. We will allow other rules to apply, but we need to
            //    make sure we don't mess with the beginning of the line that is styled with open_rule.
            //
            // If we do not have a match:
            //    This means open_rule does not end on this and will remain open. We take a shortcut in this
            //    case and style the entire line with open_rule.
            if (matches[1] != -1) {
                // We have a match!

                // Style attrs up until matches[1]
ATTO_DEBUG_PRINTF("%s\n", "open_rule found");
                for (col = 0; col < matches[1]; col++) char_attrs[col] = open_rule->attrs;
                style_from_col = matches[1]; // Don't let other rules mess with chars before matches[1]

                // Close open_rule
                bline->open_rule = NULL;
                open_rule = NULL;
            } else {
                // No match

                // This entire is styled by open_rule
ATTO_DEBUG_PRINTF("%s\n", "open_rule not found");
                for (col = 0; col < bline->length; col++) char_attrs[col] = open_rule->attrs;

                // Leave open_rule open
                bline->open_rule = open_rule;

                // We're done styling this line
                goto reduce_char_attrs;
            }
        } else if (bline->open_rule) {
            bline->open_rule = NULL;
        }

        // Apply style rules
        LL_FOREACH(self->styles, node) {
            re_offset = 0;
            rule = node->rule;
            if (rule->type == ATTO_SRULE_TYPE_SINGLE) {
                // Apply single line style rule
                while (re_offset < bline->length) {
                    if ((rc = pcre_exec(rule->cregex, NULL, self->data + bline->offset, bline->length, re_offset, 0, matches, 3)) < 0) {
if (re_offset > 0) ATTO_DEBUG_PRINTF("No match for rule[%s] at offset=%d len=%d rc=%d\n", rule->regex, re_offset, bline->length - re_offset, rc);
                        // No match
                        break;
                    }
                    // Match! Style matches[0] until matches[1] with rule->attrs as long as we're past style_from_col
                    if (matches[0] >= style_from_col) {
ATTO_DEBUG_PRINTF("This rule touched: %s [%d:%d] with %d\n", rule->regex, matches[0], matches[1] - 1, rule->attrs);
                        for (col = matches[0]; col < matches[1]; col++) char_attrs[col] = rule->attrs;
                    }
                    re_offset = matches[1]; // Advance regex cursor
                }
            } else if (rule->type == ATTO_SRULE_TYPE_MULTI) {
                // Apply multi line style rule
                while (re_offset < bline->length) {
                    if (pcre_exec(rule->cregex, NULL, self->data + bline->offset, bline->length, re_offset, 0, matches, 3) < 0) {
                        // No match
                        break;
                    }
                    // Match! Are we before style_from_col?
                    if (matches[0] < style_from_col) {
                        // This part of the line is already styled; continue on
                        re_offset = matches[0]; // Advance regex cursor
                        continue;
                    }
                    // Now find cregex_end
                    multi_start = matches[0];
                    re_offset = matches[1]; // Advance regex_cursor
                    if (pcre_exec(rule->cregex_end, NULL, self->data + bline->offset, bline->length, re_offset, 0, matches, 3) < 0) {
                        // No match for cregex_end; it might end on another line

                        // Style the rest of the line with rule
ATTO_DEBUG_PRINTF("No match for cregex_end, setting line[%d].open_rule to %p\n", line, rule);
if (bline->length > 100) {
ATTO_DEBUG_PRINTF("big len=%d\n", bline->length);
endwin();
exit(1);
}
                        for (col = multi_start; col < bline->length; col++) char_attrs[col] = rule->attrs;

                        // Leave rule open
                        open_rule = rule;
                        bline->open_rule = rule;

                        // We're done styling this line
                        goto reduce_char_attrs;
                    } else {
                        // Match! Style multi_start until matches[1] with rule->attrs
ATTO_DEBUG_PRINTF("%s\n", "cregex -> cregex_end found");
                        for (col = multi_start; col < matches[1]; col++) char_attrs[col] = rule->attrs;
                        re_offset = matches[1]; // Advance regex cursor
                    }
                }
            } else if (rule->type == ATTO_SRULE_TYPE_RANGE) {
                // Apply range style rule

                // Account for range_end before range_start
                range_start = rule->range_start->offset < rule->range_end->offset ? rule->range_start : rule->range_end;
                range_end = range_start == rule->range_start ? rule->range_end : rule->range_start;
                if (range_start->offset >= bline->offset
                    && range_start->offset < bline->offset + bline->length
                    // TODO Should we disallow a range to apply if range_start->offset is < style_from_col?
                ) {
                    // Range starts on this line!
                    if (range_end->offset < bline->offset + bline->length) {
                        // Range ends on this line! Style range_start->col until range_end->col with rule->attrs
ATTO_DEBUG_PRINTF("%s\n", "range -> range_end found");
                        for (col = range_start->col; col < range_end->col; col++) char_attrs[col] = rule->attrs;
                    } else {
                        // Range ends on another line

                        // Style the rest of the line with rule
ATTO_DEBUG_PRINTF("%s\n", "range_end not found");
                        for (col = range_start->col; col < bline->length; col++) char_attrs[col] = rule->attrs;

                        // Leave rule open
                        open_rule = rule;
                        bline->open_rule = rule;

                        // We're done styling this line
                        goto reduce_char_attrs;
                    }
                }
            }
        }
ATTO_DEBUG_PRINTF("line=%d char_attrs[\n", line);
for (col = 0; col < bline->length; col++) ATTO_DEBUG_PRINTF("    %d\n", char_attrs[col]);
ATTO_DEBUG_PRINTF("%s\n", "]");

        // Now we need to convert char_attrs to an array of sspan_t
        // I.e., AAAABBBCDDDDDDDD -> {4,A},{3,B},{1,C},{8,D}
reduce_char_attrs: (void)self;
        attr_col = 0;
        sspans_len = 0;
        line_style_changed = 0;
        for (col = 1; col < bline->length + 1; col++) {
            if (col == bline->length || char_attrs[attr_col] != char_attrs[col]) {
                // Style attr_col thru col-1 with char_attrs[attr_col]
                sspans_len += 1;

                // Resize sspans if needed
                if (bline->sspans_size < sspans_len) {
                    bline->sspans_size += ATTO_SSPAN_RANGE_ALLOC_INCR;
                    bline->sspans = (sspan_t*)realloc(bline->sspans, sizeof(sspan_t) * bline->sspans_size);
                    line_style_changed = 1;
                }

                // Fill in span
                if (bline->sspans[sspans_len - 1].length != col - attr_col
                    || bline->sspans[sspans_len - 1].attrs != char_attrs[attr_col]
                ) {
                    bline->sspans[sspans_len - 1].length = col - attr_col;
                    bline->sspans[sspans_len - 1].attrs = char_attrs[attr_col];
                    line_style_changed = 1;
                }

                // Advance attr_col
                attr_col = col;
            }
        }

        if (sspans_len != bline->sspans_len) {
            bline->sspans_len = sspans_len;
            line_style_changed = 1;
        }

        if (line != dirty_line && bail_on_matching_style && bline->sspans_len > 0 && !line_style_changed) {
            // TODO fix this or use exact comparison instead of hash
            // This line is styled the same way as it was before.
            // We're done!
            break;
        }

char* bull;
buffer_get_line(self, line, 0, NULL, 0, &bull, NULL);
ATTO_DEBUG_PRINTF("line=%d str=[%s] blinep=%p spansp=%p sspans[\n", line, bull, bline, bline->sspans);
for (col = 0; col < sspans_len; col++) ATTO_DEBUG_PRINTF("    len=%d attrs=%d\n", bline->sspans[col].length, bline->sspans[col].attrs);
ATTO_DEBUG_PRINTF("%s\n", "]");
free(bull);

    }

    // Free char_attrs
    if (char_attrs) {
        free(char_attrs);
    }

    return ATTO_RC_OK;
}

/**
 * Destroy and free a buffer
 */
int buffer_destroy(buffer_t* self) {
    // TODO destroy
    return ATTO_RC_OK;
}
