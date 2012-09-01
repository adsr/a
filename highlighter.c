#include <string.h>
#include <pcre.h>

#include "highlighter.h"
#include "buffer.h"
#include "util.h"

extern FILE* fdebug;
syntax_t* syntaxes = NULL;

highlighter_t* highlighter_new(buffer_t* buffer, syntax_t* syntax) {

    highlighter_t* highlighter = (highlighter_t*)calloc(1, sizeof(highlighter_t));

    highlighter->syntax = syntax;
    highlighter->highlighted_substrs = (highlighted_substr_t*)calloc(MAX_HIGHLIGHTED_SUBSTRS, sizeof(highlighted_substr_t));

    buffer_add_listener(buffer, highlighter_on_dirty_lines, (void*)highlighter);

    return highlighter;
}

highlighted_substr_t* highlighter_highlight(highlighter_t* self, char* line, int line_in_buffer, int* substr_count) {

    syntax_rule_single_t* cur_rule;
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

    cur_rule = self->syntax->rule_single_head;
    line_length = strlen(line);

    // TODO multi-line highlighting
    temp_workspace_substr = (workspace_substrs + workspace_substrs_count);
    temp_workspace_substr->attrs = default_attrs;
    temp_workspace_substr->substr = line;
    temp_workspace_substr->start_offset = 0;
    temp_workspace_substr->end_offset = line_length - 1;
    temp_workspace_substr->length = line_length;
    workspace_substrs_count += 1;

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

fprintf(fdebug, "rule=%s match so=%d eo=%d\n", cur_rule->regex_str, ovector[0], ovector[1]);

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
    highlighter_t* self = (highlighter_t*)listener;
}

syntax_rule_single_t* syntax_rule_single_new(char* regex, int attrs) {
    syntax_rule_single_t* rule = calloc(1, sizeof(syntax_rule_single_t));
    rule->regex = util_pcre_compile(regex, NULL, NULL);
    rule->regex_str = regex;
    rule->attrs = attrs;
    return rule;
}

syntax_rule_multi_t* syntax_rule_multi_new(char* regex_start, char* regex_end, int attrs) {
    syntax_rule_multi_t* rule = calloc(1, sizeof(syntax_rule_multi_t));
    rule->regex_start = util_pcre_compile(regex_start, NULL, NULL);
    rule->regex_end = util_pcre_compile(regex_end, NULL, NULL);
    rule->attrs = attrs;
    return rule;
}
