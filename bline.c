bline_t* bline_new(buffer_t* buffer, char* content, int n) {
    bline_t* self;
    self = (bline_t*)calloc(1, sizeof(bline_t));
    self->buffer = buffer;
    if (content) {
        if (n < 1) {
            n = strlen(content);
        }
        bline_insert(self, 0, content, n, NULL, NULL);
    }
    return self;
}

char* bline_get_content(bline_t* self) {
    return self->content;
}

char* bline_get_range(bline_t* self, int col, int length) {
    char* range;
    range = (char*)calloc(length + 1, sizeof(char));
    if (col < 0 || col >= self->length) {
        return range;
    }
    if (length > self->length - col) {
        length = self->length - col;
    }
    memcpy(range, self->content + col, length);
    return range;
}

int bline_get_length(bline_t* self) {
    return self->length;
}

bline_t* bline_get_next(bline_t* self) {
    if (self->next) {
        return self->next;
    }
    return self;
}

bline_t* bline_get_prev(bline_t* self) {
    if (self->prev) {
        return self->prev;
    }
    return self;
}

int bline_insert(bline_t* self, int col, char* str, int n, bline_t** ret_line, int* ret_col) {
    char* newline;
    char* strstop;
    char* chunk;
    char* chunkstop;
    char* newchunk;
    int chunklen;
    int cursorcol;
    bline_t* bline;
    bline_t* newbline;
    int newlines_added;

    // Keep track of recursion
    static int nestlevel = 0;
    nestlevel++;

    // Normalize col param
    if (col > self->length) {
        col = self->length;
    }
    if (col < 0) {
        col = 0;
    }

    // Normalize n param (length of str)
    if (n < 0) {
        n = strlen(str);
    }

    // Exit early if n==0
    if (n == 0) {
        if (ret_line && ret_col) {
            *ret_line = self;
            *ret_col = col;
        }
        return ATTO_RC_OK;
    }

    // Find newline
    newbline = NULL;
    strstop = str + n;
    newline = strchr(str, '\n');

    if (!newline) {
        // No newlines; just insert in self->content
        _bline_insert_mem(self, col, str, n);
    } else {
        // We need to insert new line(s)
        bline = self;
        chunk = str;
        newlines_added = 0;
        cursorcol = col;
        for (;;) {
            chunkstop = newline ? newline : strstop;
            chunklen = chunkstop - chunk;
            if (chunklen > 0) {
                // Insert content chunk[chunk:chunkstop - 1] @ bline->content[cursorcol]
                _bline_insert_mem(bline, cursorcol, chunk, chunklen);
                cursorcol += chunklen;
            }

            if (newline) {
                // Insert newbline with content bline->content[cursorcol:-] after bline
                chunklen = bline->length - cursorcol;
                newchunk = (char*)calloc(chunklen + 1, sizeof(char));
                if (chunklen > 0) {
                    memcpy(newchunk, bline->content + cursorcol, chunklen);
                }
                newbline = bline_new(bline->buffer, newchunk, -1);
                newbline->linenum = bline->linenum + 1;
                if (bline->next) {
                    DL_PREPEND_ELEM(bline->buffer->blines, bline->next, newbline);
                } else {
                    DL_APPEND(bline->buffer->blines, newbline);
                }
                bline->length = cursorcol; // Truncate previous line
                bline = newbline; // Update bline
                cursorcol = 0; // Update cursorcol
                newlines_added += 1;
            } else {
                cursorcol = chunkstop - chunk;
            }

            // Prep for next loop
            if (!newline) {
                break;
            }
            chunk = newline + 1;
            if (chunk >= strstop) {
                break;
            }
            newline = strchr(chunk, '\n');
        }
    }

    // Update byte and line count, linenums, srules, marks
    nestlevel--;
    if (nestlevel == 0) {
        self->buffer->byte_count += n;
        self->buffer->line_count += newlines_added;
        if (newbline && newbline->next) {
            _buffer_update_linenums(self->buffer, newbline->next, newbline->linenum + 1);
        }
        _buffer_update_srules(self->buffer, self, col, n, str);
        _buffer_update_marks(self->buffer, self, col, n);
    }

    if (ret_line && ret_col) {
        // Return cursor
        *ret_line = bline;
        *ret_col = col;
    }

    return ATTO_RC_OK;
 }

int bline_delete(bline_t* self, int col, int num) {
    return ATTO_RC_ERR;
}

int bline_destroy(bline_t* self) {
    return ATTO_RC_ERR;
}

inline int _bline_insert_mem(bline_t* self, int col, char* str, int n) {
    if (n < 1) {
        return ATTO_RC_OK;
    }
    self->content = (char*)realloc(self->content, sizeof(char) * (self->length + n));
    if (col < self->length) {
        memmove(self->content + col + n, self->content + col, n);
    }
    memcpy(self->content + col, str, n);
    self->length = self->length + n;
    return ATTO_RC_OK;
}
