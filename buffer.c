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
    buffer->line_offsets = (int*)calloc(ATTO_LINE_OFFSET_ALLOC_INCR, sizeof(int));
    buffer->line_offsets_size = ATTO_LINE_OFFSET_ALLOC_INCR;
    // TODO check calloc retvals
    // TODO line_style_hashes, line_spans, styles
    return buffer;
}

/**
 * Read contents of filename into buffer
 */
int buffer_read(buffer_t* self, char* filename) {
    FILE* f;
    int filesize;
    char* data;

    // Open file
    f = fopen(filename, "rb");
    if (!f) {
        ATTO_DEBUG_PRINTF("Could not open file for reading: %s\n", filename);
        return ATTO_RC_ERR;
    }

    // Seek to end of file
    fseek(f, 0, SEEK_END);

    // Ask for position
    filesize = ftell(f);

    // Go back to beginning of file
    rewind(f);

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

    // Update filename
    self->filename = strdup(filename);

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

    // Update filename
    self->filename = strdup(filename);

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

    ATTO_DEBUG_PRINTF("insert data [%s]:%d at offset=%d\n", str, len, offset);

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

    // Update metadata
    _buffer_update_metadata(self, offset, line, col, delta, len * -1);

    // TODO invoke listeners
    return ATTO_RC_OK;
}

int buffer_get_line(buffer_t* self, int line, int from_col, char* usebuf, int usebuf_len, char** ret_line, int* ret_len) {
    int line_offset;
    int line_len;

    // Calc line offset and length
    line_offset = buffer_get_offset(self, line, from_col);
    buffer_get_line_col(self, line_offset, &line, &from_col);

    if (line == self->line_count - 1) {
        line_len = (self->byte_count - self->line_offsets[self->line_count]) - from_col;
    } else {
        line_len = (buffer_get_offset(self, line + 1, 0) - 1) - line_offset;
        line_len = ATTO_MAX(line_len, 0);
    }
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
    usebuf[len] = '\0';

    // Set return values
    if (ret_substr) {
        *ret_substr = usebuf;
    }
    if (ret_len) {
        *ret_len = len;
    }

    return ATTO_RC_OK;
}

int _buffer_get_line_spans(buffer_t* self, int line, char** ret_line, int* ret_len, sspan_t* ret_spans) {
    // TODO get line spans
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
        if (self->line_offsets[line] == offset) {
            if (ret_line) *ret_line = line;
            if (ret_col) *ret_col = 0;
            return ATTO_RC_OK;
        } else if (self->line_offsets[line] > offset) {
            if (ret_line) *ret_line = ATTO_MAX(line - 1, 0);
            if (ret_col) *ret_col = ATTO_MAX(offset - prev_offset, 0);
            return ATTO_RC_OK;
        }
        prev_offset = self->line_offsets[line];
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
    offset = self->line_offsets[line];

    // Make sure col is not past end of line
    if (line == self->line_count - 1) {
        col = ATTO_MIN(col, self->byte_count - offset);
    } else {
        col = ATTO_MIN(col, (self->line_offsets[line + 1] - 1) - offset);
    }

    return offset + col;
}

/**
 * Find needle in buffer from offset
 * Returns offset of match, or -1 if not found
 */
int buffer_search(buffer_t* self, char* needle, int offset) {
    int needle_len;
    int scan_len;
    char* match;
    int match_offset;

    // Clamp offset
    offset = ATTO_MAX(ATTO_MIN(offset, self->byte_count - 1), 0);

    // Calc scan length
    scan_len = self->byte_count - offset;

    // Find match
    needle_len = strlen(needle);
    if (needle_len == 1) {
        match = (char*)memchr(self->data + offset, *needle, scan_len);
    } else {
        match = (char*)memmem(self->data + offset, scan_len, needle, needle_len);
    }

    if (!match) {
        // Not found
        return -1;
    }

    // Return match offset
    return match - self->data;
}

int buffer_search_reverse(buffer_t* self, char* needle, int offset) {
    // TODO search reverse
}

int buffer_add_style(buffer_t* self, srule_t* style) {
    // TODO add style
    return ATTO_RC_OK;
}

int buffer_remove_style(buffer_t* self, srule_t* style) {
    // TODO remove style
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
    char* all;

    _buffer_update_line_offsets(self, line);
    _buffer_update_marks(self, offset, delta_len);
    _buffer_update_styles(self, line, delta, delta_len);
    _buffer_notify_listeners(self, line, col, delta, delta_len);

    buffer_get_substr(self, 0, -1, NULL, 0, &all, NULL);
    ATTO_DEBUG_PRINTF("linec=%d bytec=%d dsize=%d data=[%s]\n", self->line_count, self->byte_count, self->data_size, all);
    if (all) free(all);


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

int _buffer_update_line_offsets(buffer_t* self, int dirty_line) {
    int line;
    int offset;
    char* newline;

    // TODO optimize if we know no newlines were added

    // Sanitize input
    dirty_line = ATTO_MIN(self->line_count - 1, ATTO_MAX(0, dirty_line));

    // Start offset of dirty_line
    offset = self->line_offsets[dirty_line];

    // Refresh line offsets of lines after dirty_line
    while (offset <= self->byte_count) {
        newline = (char*)memchr(self->data + offset, '\n', self->byte_count - offset);
        if (!newline) {
            // No more newlines
            break;
        } else {
            dirty_line += 1;
            // Expand line_offsets if needed
            if (dirty_line >= self->line_offsets_size) {
                self->line_offsets_size += ATTO_LINE_OFFSET_ALLOC_INCR;
                self->line_offsets = (int*)realloc(
                    self->line_offsets,
                    sizeof(int) * self->line_offsets_size
                );
            }
            // Calculate line offset (1 byte after the newline we just found)
            offset = (newline + 1) - self->data;
            self->line_offsets[dirty_line] = offset;
        }
    }

    // Update line_count
    self->line_count = dirty_line + 1; // dirty_line is 0-indexed so add 1

    return ATTO_RC_OK;
}

int _buffer_update_marks(buffer_t* self, int offset, int delta) {
    mark_t* mark;
    int line;
    int col;

    // Update all marks gte offset
    LL_FOREACH(self->marks, mark) {
        if (mark->offset >= offset) {
            mark_move(mark, delta);
            ATTO_DEBUG_PRINTF("mark moved %d to %d [%d:%d]\n", delta, mark->offset, mark->line, mark->col);
        } else {
            ATTO_DEBUG_PRINTF("%s\n", "good");
        }
    }

    return ATTO_RC_OK;
}

int _buffer_update_styles(buffer_t* self, int dirty_line, char* delta, int delta_len) {
    srule_node_t* node;

    // TODO update styles
    LL_FOREACH(self->styles, node) {
    }

    return ATTO_RC_OK;
}

int buffer_destroy(buffer_t* self) {
    // TODO destroy
    return ATTO_RC_OK;
}
