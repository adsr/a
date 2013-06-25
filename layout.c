#include "atto.h"

/**
 * Init bview layout
 */
int _layout_init(lua_State* L, buffer_t* buffer) {
    g_bview_edit = bview_new(buffer, 0);
    lua_pushlightuserdata(L, g_bview_edit);
    lua_setglobal(L, "bview_edit");

    g_bview_status = bview_new(NULL, 1);
    lua_pushlightuserdata(L, g_bview_status);
    lua_setglobal(L, "bview_status");

    g_bview_prompt = bview_new(NULL, 1);
    lua_pushlightuserdata(L, g_bview_prompt);
    lua_setglobal(L, "bview_prompt");

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
    bview_resize(g_bview_prompt, 0, height - 1, width, 1);
    return ATTO_RC_OK;
}
