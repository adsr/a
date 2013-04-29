#define ATTO_TEST_PASS NULL

#define ATTO_TEST_ASSERT(EXPR, MSG) do { \
    if (!(EXPR)) return MSG; \
} while(0)

#define ATTO_TEST_RUN(ANYFAILS, FAILMSG, NAME) do { \
    FAILMSG = test_ ## NAME(); \
    ANYFAILS |= FAILMSG ? 1 : 0; \
    printf("%32s \x1b[%dm%s\x1b[0m %s\n", #NAME, FAILMSG ? 31 : 32, FAILMSG ? "FAIL" : "PASS", FAILMSG ? FAILMSG : ""); } while(0)

#define ATTO_TEST_CONTENT_A \
    "\"What do you want me to do?\" he whispers to the empty air.\n" \
    "It's hard to know.\n" \
    "Oh Jimmy, you were so funny.\n" \
    "Don't let me down.\n" \
    "From habit he lifts his watch; it shows him its blank face.\n" \
    "Zero hour, Snowman thinks. Time to go.\n"

char* rasprintf(const char* fmt, ...) {
    char* str;
    va_list args;
    va_start(args, fmt);
    va_end(args);
    vasprintf(&str, fmt, args);
    return str;
}

char* test_bline_insert() {
    buffer_t* buffer;
    bline_t* bline;
    int i;
    char* c;
    buffer = buffer_new();
    bline_insert(buffer->blines, 0, ATTO_TEST_CONTENT_A, -1, &bline, &i);
    ATTO_TEST_ASSERT(bline->linenum == 6, rasprintf("cursor linenum expected=7 observed=%d", bline->linenum));
    ATTO_TEST_ASSERT(i == 0, rasprintf("cursor col expected=0 observed=%d", i));
    i = 0;
    DL_FOREACH(buffer->blines, bline) {
        ATTO_TEST_ASSERT(bline->linenum == i, rasprintf("cursor linenum expected=%d observed=%d", i, bline->linenum));
        i++;
    }
    c = buffer_get_content(buffer);
    ATTO_TEST_ASSERT(!strcmp(ATTO_TEST_CONTENT_A, c), rasprintf("content expected=[%s] observed=[%s]", ATTO_TEST_CONTENT_A, c));
    return ATTO_TEST_PASS;
}

int test_run() {
    int anyfails;
    char* failmsg;
    anyfails = 0;
    ATTO_TEST_RUN(anyfails, failmsg, bline_insert);
    return anyfails;
}
