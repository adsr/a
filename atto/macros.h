#ifndef __MACROS_H
#define __MACROS_H

#define ATTO_RC_OK 0
#define ATTO_RC_ERR -1
#define ATTO_RETURN_OK do { \
    return ATTO_RC_OK; \
} while (0)
#define ATTO_RETURN_ERR(fmt, ...) do { \
    printf(fmt, __VA_ARGS__); \
    exit(1); \
    return ATTO_RC_ERR; \
} while (0)

#define ATTO_MARK_MAX_NAME_LEN 16

#define ATTO_FUNCTION(name) int name(editor_t* editor, keyinput_t* keyinput, bview_t* bview, buffer_t* buffer, cursor_t* cursor, bline_t* bline, size_t offset)
#define ATTO_FUNCTION_ARGS(editor, keyinput) editor, keyinput, editor->active, editor->active->buffer, editor->active->active_cursor, editor->active->active_cursor->mark->bline, editor->active->active_cursor->mark->char_offset

#define ATTO_SRULE_TYPE_SINGLE 0
#define ATTO_SRULE_TYPE_MULTI 1
#define ATTO_SRULE_TYPE_RANGE 2

#define ATTO_BACTION_TYPE_INSERT 0
#define ATTO_BACTION_TYPE_DELETE 1

#define ATTO_SBLOCK_FG(s) ((s) & 0x000000ff)
#define ATTO_SBLOCK_BG(s) ((s) & 0x0000ff00)

#define ATTO_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define ATTO_MAX(a,b) (((a) > (b)) ? (a) : (b))

#ifndef ATTO_DEBUG
#define ATTO_DEBUG 1
#endif
#define ATTO_DEBUG_PRINTF(fmt, ...) do { \
    if (ATTO_DEBUG) { \
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(editor->tdebug)); \
        fprintf(editor->fdebug, "%ld [%s] ", (editor->tdebug).tv_nsec, __PRETTY_FUNCTION__); \
        fprintf(editor->fdebug, (fmt), __VA_ARGS__); \
        fflush(editor->fdebug); \
    } \
} while (0)

#endif
