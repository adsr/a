#include <stdlib.h>
#include <string.h>
#include <pcre.h>

#include "highlighter.h"
#include "buffer.h"
#include "util.h"

#include "ext/uthash/uthash.h"

extern FILE* fdebug;
syntax_t* syntaxes = NULL;
UT_icd syntax_rule_multi_range_icd = { sizeof(syntax_rule_multi_range_t), NULL, NULL, syntax_rule_multi_range_dtor };
int syntax_rule_id = 1;

highlighter_t* highlighter_new(buffer_t* buffer, syntax_t* syntax) {

    highlighter_t* highlighter = (highlighter_t*)calloc(1, sizeof(highlighter_t));

    highlighter->syntax = syntax;
    highlighter->highlighted_substrs = (highlighted_substr_t*)calloc(MAX_HIGHLIGHTED_SUBSTRS, sizeof(highlighted_substr_t));
    highlighter->buffer = buffer;

    buffer_add_listener(buffer, highlighter_on_dirty_lines, (void*)highlighter);

    return highlighter;
}

highlighted_substr_t* highlighter_highlight(highlighter_t* self, char* line, int line_in_buffer, int* substr_count) {

    syntax_rule_single_t* cur_rule;
    syntax_rule_multi_t* cur_multi_rule;
    int line_length;
    int rc;
    int i;
    int j;
    int start_offset = 0;
    static int ovector[3];
    static int default_attrs = -1;
    highlighted_substr_t* temp_workspace_substr = NULL;
    highlighted_substr_t* current_workspace_substr = NULL;
    highlighted_substr_t* current_substr = NULL;
    static highlighted_substr_t* workspace_substrs = NULL;
    int workspace_substrs_count = 0;
    syntax_rule_multi_workspace_t* multi_workspace = NULL;
    syntax_rule_multi_range_t* multi_range = NULL;
    bool is_adhoc_iter = FALSE;

    if (default_attrs == -1) {
         default_attrs = util_ncurses_getpair("default_fg", "default_bg");
    }
    if (workspace_substrs == NULL) {
        workspace_substrs = (highlighted_substr_t*)calloc(MAX_HIGHLIGHTED_SUBSTRS, sizeof(highlighted_substr_t));
    }

    *substr_count = 0;

    if (self->syntax == NULL) {
        return NULL;
    }

    line_length = strlen(line);

    // Default highlighting rule
    temp_workspace_substr = (workspace_substrs + workspace_substrs_count);
    temp_workspace_substr->attrs = default_attrs;
    temp_workspace_substr->substr = line;
    temp_workspace_substr->start_offset = 0;
    temp_workspace_substr->end_offset = line_length - 1;
    temp_workspace_substr->length = line_length;
    workspace_substrs_count += 1;

    // Single-line highlighting rules
    cur_rule = self->syntax->rule_single_head;
    while (1) {
        while (cur_rule != NULL) {
            start_offset = 0;
            while (1) {
                rc = pcre_exec(
                    cur_rule->regex,
                    NULL,
                    line,
                    line_length,
                    start_offset,
                    0,
                    ovector,
                    3
                );

                if (rc < 1) {
                    break;
                }

                temp_workspace_substr = (workspace_substrs + workspace_substrs_count);
                temp_workspace_substr->attrs = cur_rule->attrs;
                temp_workspace_substr->substr = line + ovector[0];
                temp_workspace_substr->start_offset = ovector[0];
                temp_workspace_substr->end_offset = ovector[1] - 1;
                temp_workspace_substr->length = ovector[1] - ovector[0];
                workspace_substrs_count += 1;

                start_offset = ovector[1];
                if (start_offset >= line_length) {
                    break;
                }

            }
            cur_rule = cur_rule->next;
        }
        if (is_adhoc_iter) {
            break;
        } else {
            // Second pass will be adhoc rules
            cur_rule = self->buffer->rule_adhoc_head;
            is_adhoc_iter = TRUE;
        }
    }

    // Multi-line highlighting rules
    cur_multi_rule = self->syntax->rule_multi_head;
    while (cur_multi_rule != NULL) {
        multi_workspace = syntax_rule_multi_workspace_find_or_new(cur_multi_rule, self->buffer);
        multi_range = (syntax_rule_multi_range_t*)utarray_eltptr(multi_workspace->ranges, line_in_buffer);
        while (multi_range != NULL && multi_range->active) {
            temp_workspace_substr = (workspace_substrs + workspace_substrs_count);
            temp_workspace_substr->attrs = cur_multi_rule->attrs;
            temp_workspace_substr->substr = line + multi_range->start_offset;
            temp_workspace_substr->start_offset = multi_range->start_offset;
            temp_workspace_substr->end_offset = multi_range->end_offset;
            temp_workspace_substr->length = multi_range->end_offset - multi_range->start_offset;
            workspace_substrs_count += 1;
            multi_range = multi_range->next;
        }
        cur_multi_rule = cur_multi_rule->next;
    }

    // Flatten rule matches
    for (i = 0; i < line_length; i++) {
        for (j = workspace_substrs_count - 1; j >= 0; j--) {
            temp_workspace_substr = (workspace_substrs + j);
            if (i >= temp_workspace_substr->start_offset && i <= temp_workspace_substr->end_offset) {
                if (current_workspace_substr != temp_workspace_substr) {
                    if (current_substr != NULL) {
                        current_substr->end_offset = i - 1;
                        current_substr->length = current_substr->end_offset - current_substr->start_offset + 1;
                    }
                    current_substr = (self->highlighted_substrs + *substr_count);
                    current_substr->start_offset = i;
                    current_substr->attrs = temp_workspace_substr->attrs;
                    current_substr->substr = line + i;
                    current_workspace_substr = temp_workspace_substr;
                    *substr_count += 1;
                }
                break;
            }
        }
    }
    if (current_substr != NULL) {
        current_substr->end_offset = line_length - 1;
        current_substr->length = current_substr->end_offset - current_substr->start_offset + 1;
    }

    return self->highlighted_substrs;
}

void highlighter_on_dirty_lines(buffer_t* buffer, void* listener, int line_start, int line_end, int line_delta) {

    int line;
    bool overlaps_with_multi_range;
    highlighter_t* self = (highlighter_t*)listener;
    int buffer_offset = buffer_get_buffer_offset(buffer, line_start, 0);
    syntax_rule_multi_t* cur_rule = self->syntax->rule_multi_head;
    syntax_rule_multi_workspace_t* workspace;
    syntax_rule_multi_range_t* range;

    while (cur_rule != NULL) {
        workspace = syntax_rule_multi_workspace_find_or_new(cur_rule, buffer);
        overlaps_with_multi_range = FALSE;
        for (line = line_start; line <= line_end; line++) {
            range = utarray_eltptr(workspace->ranges, line);
            if (range && range->active) {
                overlaps_with_multi_range = TRUE;
                break;
            }
        }
        if (overlaps_with_multi_range || syntax_rule_multi_lines_contain_match(cur_rule, buffer, line_start, line_end)) {
            highlighter_update_offsets(buffer, cur_rule->regex_start, buffer_offset, workspace->start_offsets, FALSE);
            highlighter_update_offsets(buffer, cur_rule->regex_end, buffer_offset, workspace->end_offsets, TRUE);
            highlighter_update_lines(workspace, line_start, line_end);
        }
        cur_rule = cur_rule->next;
    }

}

int highlighter_update_offsets(buffer_t* buffer, pcre* regex, int look_offset, UT_array* offsets, bool is_end_offset) {

    int* offset;
    int offset_index = 0;
    int rc;
    static int ovector[3];
    int offsets_len = utarray_len(offsets);
    look_offset = 0;

    while (0) {
        if (offset_index < offsets_len) {
            offset = (int*)utarray_eltptr(offsets, offset_index);
            if (*offset < look_offset) {
                offset_index += 1;
                continue;
            }
        }
        break;
    }

    while (1) {
        rc = pcre_exec(
            regex,
            NULL,
            buffer->buffer->data,
            buffer->char_count,
            look_offset,
            0,
            ovector,
            3
        );
        if (rc < 1) {
            break;
        }

        if (offset_index >= offsets_len) {
            utarray_resize(offsets, offset_index + 1);
            offsets_len = offset_index + 1;
        }
        offset = (int*)utarray_eltptr(offsets, offset_index);
        if (is_end_offset) {
            *offset = ovector[1] - 1;
        } else {
            *offset = ovector[0];
        }

        look_offset = ovector[1];
        offset_index += 1;

    }

    utarray_resize(offsets, offset_index);

}

/**
 * Calculate workspace->ranges using workspace->(start|end)_offsets
 *
 * @param syntax_rule_multi_workspace_t* workspace
 */
int highlighter_update_lines(syntax_rule_multi_workspace_t* workspace, int line_start, int line_end) { // TODO optimize

    int i;
    int line;
    int start_offsets_i;
    int end_offsets_i;
    int start_offsets_len;
    int end_offsets_len;
    int range_start_offset;
    int range_end_offset;
    int range_start_line;
    int range_end_line;
    int range_start_line_offset;
    int range_end_line_offset;
    int* temp;
    bool found;
    int line_length;
    syntax_rule_multi_range_t* range;
    bool was_active;
    buffer_t* buffer = workspace->buffer;

    // Reset ranges
    utarray_resize(workspace->ranges, buffer->line_count);
    for (i = 0; i < buffer->line_count; i++) {
        range = (syntax_rule_multi_range_t*)utarray_eltptr(workspace->ranges, i);
        was_active = range->active;
        syntax_rule_multi_range_dtor((void*)range);
        if (was_active) {
            control_buffer_view_dirty_lines(control_get_active_buffer_view(), i, i, 0);
        }
    }

    // Find ranges using (start|end)_offsets
    // This assumes that (start|end)_offsets are sorted
    start_offsets_len = utarray_len(workspace->start_offsets);
    start_offsets_i = 0;
    end_offsets_len = utarray_len(workspace->end_offsets);
    end_offsets_i = 0;
    range_start_offset = -1;
    range_end_offset = -1;

    while (start_offsets_i < start_offsets_len && end_offsets_i < end_offsets_len) {

        // Find next start_offsets value greater than range_end_offset
        found = FALSE;
        do {
            temp = (int*)utarray_eltptr(workspace->start_offsets, start_offsets_i);
            start_offsets_i += 1;
            if (*temp > range_end_offset) {
                range_start_offset = *temp;
                found = TRUE;
                break;
            }
        } while (start_offsets_i < start_offsets_len);
        if (!found) {
            break;
        }

        // Find next end_offsets value greater than range_start_offset
        found = FALSE;
        do {
            temp = (int*)utarray_eltptr(workspace->end_offsets, end_offsets_i);
            end_offsets_i += 1;
            if (*temp > range_start_offset) {
                range_end_offset = *temp;
                found = TRUE;
                break;
            }
        } while (end_offsets_i < end_offsets_len);
        if (!found) {
            break;
        }

        // Woohoo! range_start_offset thru range_end_offset should be highlighted
        buffer_get_line_and_offset(buffer, range_start_offset, &range_start_line, &range_start_line_offset);
        buffer_get_line_and_offset(buffer, range_end_offset, &range_end_line, &range_end_line_offset);

        // Add ranges
        if (range_start_line == range_end_line) {
            syntax_rule_multi_add_range(workspace, range_start_line, range_start_line_offset, range_end_line_offset);
        } else { // range_start_line != range_end_line
            for (line = range_start_line; line <= range_end_line; line++) {
                if (line != range_end_line) {
                    buffer_get_line_offset_and_length(buffer, line, temp, &line_length);
                }
                if (line == range_start_line) {
                    syntax_rule_multi_add_range(workspace, line, range_start_line_offset, line_length - 1);
                } else if (line == range_end_line) {
                    syntax_rule_multi_add_range(workspace, line, 0, range_end_line_offset);
                } else {
                    syntax_rule_multi_add_range(workspace, line, 0, line_length - 1);
                }
            }
        }

        // Trigger line refresh
        for (line = range_start_line; line <= range_end_line; line++) {
            if (line < line_start || line > line_end) {
                // TODO this is broken; loop through all buffer_views associated with buffer and refresh them
                control_buffer_view_dirty_lines(control_get_active_buffer_view(), line, line, 0);
            }
        }

    }

    return 0;
}

syntax_rule_single_t* syntax_rule_single_new(char* regex, int attrs) {
    syntax_rule_single_t* rule = calloc(1, sizeof(syntax_rule_single_t));
    syntax_rule_single_edit(rule, regex, attrs);
    rule->syntax_rule_id = syntax_rule_id;
    syntax_rule_id += 1;
    return rule;
}

int syntax_rule_single_edit(syntax_rule_single_t* rule, char* regex, int attrs) {
    if (rule->regex) {
        pcre_free(rule->regex);
    }
    if (rule->regex_str) {
        free(rule->regex_str);
    }
    rule->regex = util_pcre_compile(regex, NULL, NULL);
    rule->regex_str = strdup(regex);
    rule->attrs = attrs;
    return 0;
}

syntax_rule_multi_t* syntax_rule_multi_new(char* regex_start, char* regex_end, int attrs) {
    syntax_rule_multi_t* rule = calloc(1, sizeof(syntax_rule_multi_t));
    rule->regex_start = util_pcre_compile(regex_start, NULL, NULL);
    rule->regex_end = util_pcre_compile(regex_end, NULL, NULL);
    rule->attrs = attrs;
    rule->syntax_rule_id = syntax_rule_id;
    syntax_rule_id += 1;
    return rule;
}

syntax_rule_multi_workspace_t* syntax_rule_multi_workspace_find_or_new(syntax_rule_multi_t* rule, buffer_t* buffer) {
    syntax_rule_multi_workspace_t* workspace;
    HASH_FIND_PTR(rule->workspaces, &(buffer->key), workspace);
    if (workspace == NULL) {
        workspace = calloc(1, sizeof(syntax_rule_multi_workspace_t));
        workspace->buffer = buffer;
        workspace->buffer_key = buffer->key;
        utarray_new(workspace->start_offsets, &ut_int_icd);
        utarray_new(workspace->end_offsets, &ut_int_icd);
        utarray_new(workspace->ranges, &syntax_rule_multi_range_icd);
        HASH_ADD_PTR(rule->workspaces, buffer_key, workspace);
    }
    return workspace;
}

int syntax_rule_multi_add_range(syntax_rule_multi_workspace_t* workspace, int line, int start_offset, int end_offset) {
    syntax_rule_multi_range_t* range;
    syntax_rule_multi_range_t** range_ref;
    range = (syntax_rule_multi_range_t*)utarray_eltptr(workspace->ranges, line);
    range_ref = &range;
    while ((*range_ref)->active) {
        range_ref = &((*range_ref)->next);
        if (*range_ref == NULL) {
            break;
        }
    }
    return syntax_rule_multi_range_make(range_ref, start_offset, end_offset);
}

int syntax_rule_multi_range_make(syntax_rule_multi_range_t** range_ref, int start_offset, int end_offset) {
    if (*range_ref == NULL) {
        *range_ref = calloc(1, sizeof(syntax_rule_multi_range_t));
    }
    (*range_ref)->active = TRUE;
    (*range_ref)->start_offset = start_offset;
    (*range_ref)->end_offset = end_offset;
    return 0;
}

void syntax_rule_multi_range_dtor(void* el) {

    syntax_rule_multi_range_t* range;
    syntax_rule_multi_range_t* temp;
    bool is_first = TRUE;

    range = (syntax_rule_multi_range_t*)el;

    while (range != NULL) {
        range->active = FALSE;
        temp = range->next;
        if (!is_first) {
            //free(range); // TODO figure out seg fault
        } else {
            is_first = FALSE;
        }
        range = temp;
    }
}

bool syntax_rule_multi_lines_contain_match(syntax_rule_multi_t* rule, buffer_t* buffer, int line_start, int line_end) {

    int rc;
    int line_length;
    int end_offset;
    int start_offset;
    int ovector[3];

    start_offset = buffer_get_buffer_offset(buffer, line_start, 0);
    buffer_get_line_offset_and_length(buffer, line_end, &end_offset, &line_length);
    end_offset += line_length;

    rc = pcre_exec(
        rule->regex_start,
        NULL,
        buffer->buffer->data + start_offset,
        end_offset - start_offset,
        0,
        0,
        NULL,
        0
    );
    if (rc >= 0) {
        return TRUE;
    }

    rc = pcre_exec(
        rule->regex_end,
        NULL,
        buffer->buffer->data + start_offset,
        end_offset - start_offset,
        0,
        0,
        NULL,
        0
    );
    if (rc >= 0) {
        return TRUE;
    }

    return FALSE;
}
