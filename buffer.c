buffer_t* buffer_new() {
    buffer_t* self;
    self = (buffer_t*)calloc(1, sizeof(buffer_t));
    DL_APPEND(self->blines, bline_new(self, NULL, -1));
    self->line_count = 1;
    self->byte_count = 0;
    self->content_is_dirty = 1;
    return self;
}

int buffer_read(buffer_t* self, char* filename) {
    FILE* fp;
    char* line;
    ssize_t num_chars;
    size_t len;
    bline_t* bline;
    fp = fopen(filename, "r");
    if (!fp) {
        return ATTO_RC_ERR;
    }
    buffer_clear(self);
    line = NULL;
    len = 0;
    while ((num_chars = getline(&line, &len, fp)) != -1) {
        bline = bline_new(self, strdup(line), -1);
        DL_APPEND(self->blines, bline);
    }
    if (line) {
        free(line);
    }
    fclose(fp);
    return ATTO_RC_OK;
}

int buffer_write(buffer_t* self, char* filename, int is_append) {
    FILE* fp;
    bline_t* bline;
    fp = fopen(filename, (is_append ? "a" : "w"));
    if (!fp) {
        return ATTO_RC_ERR;
    }
    DL_FOREACH(self->blines, bline) {
        fwrite(bline->content, sizeof(char), bline->length, fp);
        fwrite("\n", sizeof(char), 1, fp);
    }
    fclose(fp);
    return ATTO_RC_OK;
}

int buffer_clear(buffer_t* self) {
    bline_t* bline;
    bline_t* blinetmp;
    srule_t* srule;
    srule_t* sruletmp;
    mark_t* mark;
    mark_t* marktmp;
    self->line_count = 0;
    self->content_is_dirty = 1;
    if (self->filename) {
        free(self->filename);
    }
    if (self->content) {
        free(self->content);
    }
    if (self->blines) {
        DL_FOREACH_SAFE(self->blines, bline, blinetmp) {
            bline_destroy(bline);
        }
    }
    if (self->srules) {
        DL_FOREACH_SAFE(self->srules, srule, sruletmp) {
            srule_destroy(srule);
        }
    }
    if (self->marks) {
        DL_FOREACH_SAFE(self->marks, mark, marktmp) {
            mark_destroy(mark);
        }
    }
    return ATTO_RC_OK;
}

int buffer_destroy(buffer_t* self) {
    buffer_clear(self);
    free(self);
    return ATTO_RC_OK;
}

char* buffer_get_content(buffer_t* self) {
    bline_t* bline;
    char* p;
    if (self->content_is_dirty) {
        self->content = (char*)realloc(self->content, sizeof(char) * self->byte_count);
        p = self->content;
        DL_FOREACH(self->blines, bline) {
            memcpy(p, bline->content, bline->length);
            p += bline->length;
            if (bline->next) {
                *p = '\n';
            }
            p++;
        }
        self->content_is_dirty = 0;
    }
    return self->content;
}

int _buffer_update_linenums(buffer_t* self, bline_t* start_line, int start_num) {
    return ATTO_RC_ERR;
}

int _buffer_update_srules(buffer_t* self, bline_t* line, int col, int delta, char* strdelta) {
    return ATTO_RC_ERR;
}

int _buffer_update_marks(buffer_t* self, bline_t* line, int col, int delta) {
    return ATTO_RC_ERR;
}
