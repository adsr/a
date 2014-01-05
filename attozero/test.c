#include "atto.h"

#define ATTO_TEST_RUN(name, retval, overall) do { \
    retval = test_##name(); \
    if (retval && overall != ATTO_RC_ERR) overall = ATTO_RC_ERR; \
    printf("%24s ... %4s %s\n", #name, !retval ? "\e[32mPASS\e[0m" : "\e[31mFAIL\e[0m", retval ? retval : ""); } while (0)

#define ATTO_TEST_ASSERT(expr, fail_msg) do { \
    if (!(expr)) return fail_msg; } while (0)

/**
 * Test basic buffer edits
 */
char* test_buffer_simple() {
    buffer_t* b;
    int line_size;
    int line_len;
    char* line;
    int ret_line;
    int ret_col;

    line_size = 1024;
    line = (char*)calloc(line_size + 1, sizeof(char));

    b = buffer_new();
    ATTO_TEST_ASSERT(b->line_count == 1, "line_count should be 1");
    ATTO_TEST_ASSERT(b->byte_count == 0, "byte_count should be 0");
    ATTO_TEST_ASSERT(b->blines[0].offset == 0, "line 0 should start at offset 0");

    buffer_get_line(b, 0, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 0, "line 0 len should be 0");

    buffer_insert(b, 0, "Test line 1", 11, NULL, NULL, &ret_col);
    ATTO_TEST_ASSERT(b->line_count == 1, "line_count should still be 1");
    ATTO_TEST_ASSERT(b->byte_count == 11, "byte_count should be 11");
    ATTO_TEST_ASSERT(b->blines[0].offset == 0, "line 0 should still start at offset 0");
    ATTO_TEST_ASSERT(ret_col == 11, "ret_col should be 11");

    buffer_get_line(b, 0, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 11, "line 0 len should be 11");
    ATTO_TEST_ASSERT(!strncmp(line, "Test line 1", line_len), "line 0 should contain: Test line 1");

    buffer_insert(b, 11, "\n", 1, NULL, NULL, NULL);
    ATTO_TEST_ASSERT(b->line_count == 2, "line_count should be 2 now");
    ATTO_TEST_ASSERT(b->byte_count == 12, "byte_count should be 12");
    ATTO_TEST_ASSERT(b->blines[1].offset == 12, "line 1 should start at offset 12");

    buffer_get_line(b, 1, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 0, "line 1 len should be 0");

    buffer_insert(b, 11, "\n", 1, NULL, NULL, NULL);
    ATTO_TEST_ASSERT(b->line_count == 3, "line_count should be 3 now");
    ATTO_TEST_ASSERT(b->byte_count == 13, "byte_count should be 13");
    ATTO_TEST_ASSERT(b->blines[2].offset == 13, "line 2 should start at offset 13");

    buffer_get_line(b, 1, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 0, "line 1 len should still be 0");

    buffer_get_line(b, 2, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 0, "line 2 len should be 0");

    buffer_insert(b, 13, "a", 1, NULL, &ret_line, &ret_col);
    ATTO_TEST_ASSERT(ret_line == 2, "ret_line should be 2");
    ATTO_TEST_ASSERT(ret_col == 1, "ret_col should be 1");

    buffer_get_line(b, 2, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 1, "line 2 len should now be 1");
    ATTO_TEST_ASSERT(!strncmp(line, "a", line_len), "line 2 should contain: a");

    buffer_insert(b, 13, "!", 1, NULL, &ret_line, &ret_col);
    buffer_get_line(b, 2, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 2, "line 2 len should now be 2");
    ATTO_TEST_ASSERT(!strncmp(line, "!a", line_len), "line 2 should contain: !a");

    buffer_delete(b, 13, 1);
    buffer_get_line(b, 2, 0, line, line_size, &line, &line_len);
    ATTO_TEST_ASSERT(line_len == 1, "line 2 len should be 1 again");

    free(line);
    buffer_destroy(b);
    return NULL;
}

/**
 * Test basic mark features
 */
char* test_mark_simple() {
    buffer_t* b;
    mark_t* m;
    char* line;
    int line_size;

    line_size = 1024;
    line = (char*)calloc(line_size + 1, sizeof(char));

    b = buffer_new();
    m = buffer_add_mark(b, 0);

    ATTO_TEST_ASSERT(b->line_count == 1, "line_count should be 1");
    ATTO_TEST_ASSERT(b->byte_count == 0, "byte_count should be 0");
    ATTO_TEST_ASSERT(b->blines[0].offset == 0, "line 0 should start at offset 0");

    // TODO test_mark_simple

    free(line);
    buffer_remove_mark(b, m);
    buffer_destroy(b);
    return NULL;
}

/**
 * Run all tests
 */
int test_run() {
    char* retval;
    int overall;

    overall = ATTO_RC_OK;
    printf("Running all tests\n\n");

    ATTO_TEST_RUN(buffer_simple, retval, overall);
    ATTO_TEST_RUN(mark_simple, retval, overall);

    return overall;
}
