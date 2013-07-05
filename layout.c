#include "atto.h"

/**
 * Init bview layout
 */
int _layout_init(lua_State* L, buffer_t* buffer) {
    g_bview_edit = bview_new(buffer, 0);
    lua_pushlightuserdata(L, g_bview_edit);
    lua_setglobal(L, "bview_edit");
    lua_pushlightuserdata(L, g_bview_edit->buffer);
    lua_setglobal(L, "buffer_edit");

    g_bview_status = bview_new(NULL, 1);
    lua_pushlightuserdata(L, g_bview_status);
    lua_setglobal(L, "bview_status");
    lua_pushlightuserdata(L, g_bview_status->buffer);
    lua_setglobal(L, "buffer_status");

    g_bview_prompt = bview_new(NULL, 1);
    lua_pushlightuserdata(L, g_bview_prompt);
    lua_setglobal(L, "bview_prompt");
    lua_pushlightuserdata(L, g_bview_prompt->buffer);
    lua_setglobal(L, "buffer_prompt");

    g_prompt_label = newwin(1, 1, 0, 0);

    return ATTO_RC_OK;
}

/**
 * Resize bview layout
 */
int _layout_resize(int width, int height) {
    bview_t* bview;
    for (bview = g_bview_edit; bview; bview = bview->next) {
        bview_resize(bview, 0, 0, width, height - 2);
    }
    bview_resize(g_bview_status, 0, height - 2, width, 1);

    mvwin(g_prompt_label, height - 1, 0);
    wresize(g_prompt_label, 1, ATTO_MAX(g_prompt_label_len, 1));
    wnoutrefresh(g_prompt_label);

    bview_resize(g_bview_prompt, g_prompt_label_len, height - 1, width - g_prompt_label_len, 1);
    return ATTO_RC_OK;
}
