local keymap_default = keymap_new(0)
keymap_bind(keymap_default, "left", function(context) mark_move(context.cursor, -1) end)
keymap_bind(keymap_default, "right", function(context) mark_move(context.cursor, 1) end)
keymap_bind(keymap_default, "up", function(context) mark_move_line(context.cursor, -1) end)
keymap_bind(keymap_default, "down", function(context) mark_move_line(context.cursor, 1) end)
keymap_bind(keymap_default, "pageup", function(context) mark_move_line(context.cursor, context.viewport_h * -1) end)
keymap_bind(keymap_default, "pagedown", function(context) mark_move_line(context.cursor, context.viewport_h) end)
keymap_bind(keymap_default, "tab", function(context)
    local num_spaces = 4 - (context.col % 4)
    buffer_insert(context.buffer, context.offset, string.rep(" ", num_spaces), num_spaces)
end)
keymap_bind(keymap_default, "CA", function(context)
    local ok, line_offset, line_len = buffer_get_line_offset_len(context.buffer, context.line, 0);
    local match_offset = buffer_regex(context.buffer, [[\S]], line_offset, line_len)
    if match_offset < 0 or match_offset == context.offset then
        match_offset = line_offset
    end
    mark_set(context.cursor, match_offset)
end)
keymap_bind(keymap_default, "CE", function(context)
    local ok, offset, len = buffer_get_line_offset_len(context.buffer, context.line, 0);
    mark_set_line_col(context.cursor, context.line, len)
end)
keymap_bind(keymap_default, "backspace", function(context)
    if context.offset > 0 then
        buffer_delete(context.buffer, context.offset - 1, 1)
    end
end)
keymap_bind(keymap_default, "delete", function(context)
    if context.offset < context.byte_count then
        local ok, line, col = mark_get(context.cursor)
        buffer_delete(context.buffer, context.offset, 1)
        mark_set_line_col(context.cursor, line, col)
    end
end)
keymap_bind(keymap_default, "enter", function(context)
    local ok, offset, line, col = buffer_insert(context.buffer, context.offset, "\n", 1)
end)

keymap_bind(keymap_default, [[C\]], function(context) bview_split(context.bview, 1, 0.5) end)
keymap_bind(keymap_default, [[C_]], function(context) bview_split(context.bview, 0, 0.5) end)

keymap_bind(keymap_default, "CO", function(context)
    prompt("Save to path: ", "default", function(response)
        atto_debug("reponse=" .. response)
        buffer_write(context.buffer, response, 0)
    end)
end)

--[[
keymap_bind(keymap_default, "CR", function(context)
    bview_set_active(bview_prompt)
end)
--]]

function prompt(label, default, callback)
    local cur_bview = bview_get_active()
    local response = ""
    local keymap_prompt = nil

    if cur_bview == bview_prompt then
        return
    end

    keymap_prompt = keymap_new(1)
    keymap_bind(keymap_prompt, "enter", function(context)
        local buf = string.rep(" ", context.byte_count)
        ok, response, response_len = buffer_get_line(buffer_prompt, 0, 0, buf, context.byte_count)
        keymap_destroy(bview_keymap_pop(bview_prompt))
        buffer_clear(buffer_prompt)
        bview_set_prompt_label("")
        bview_set_active(cur_bview)
        callback(response)
    end)

    bview_keymap_push(bview_prompt, keymap_prompt)
    bview_set_active(bview_prompt)
    bview_set_prompt_label(label)
    bview_update_cursor(bview_prompt)
    buffer_clear(buffer_prompt)
    buffer_insert(buffer_prompt, 0, default, string.len(default))
end

keymap_bind_default(keymap_default, function(context)
    local ok, offset, line, col = buffer_insert(context.buffer, context.offset, context.keyc, string.len(context.keyc))
end)

bview_keymap_push(bview_edit, keymap_default)
bview_keymap_push(bview_prompt, keymap_default)

local php_style = {
    srule_new_single([=[\$[a-zA-Z_0-9$]*|[=!<>-]]=], COLOR_GREEN, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[\b(class|var|const|static|final|private|public|protected|function|switch|case|default|endswitch|if|else|elseif|endif|for|foreach|endfor|endforeach|while|endwhile|new|die|echo|continue|exit)\b]=], COLOR_YELLOW, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[\b-?[0-9]+\b]=], COLOR_BLUE, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[\b(true|false|null)\b]=], COLOR_BLUE, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[[(){}.,;]]=], COLOR_RED, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[(\$|=>|->|::)]=], COLOR_GREEN, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[(\[|\])]=], COLOR_WHITE, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=['([^']|(\\'))*']=], COLOR_YELLOW, COLOR_DEFAULT_BG, A_BOLD),
    srule_new_single([=["([^"]|(\\"))*"]=], COLOR_YELLOW, COLOR_DEFAULT_BG, A_BOLD),
    srule_new_single([=[(#.*|//.*)$]=], COLOR_CYAN, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[(<\?(php)?|\?>)]=], COLOR_GREEN, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_single([=[\s+$]=], COLOR_DEFAULT_FG, COLOR_GREEN, A_NORMAL),
    srule_new_multi([[/\*]], [[\*/]], COLOR_CYAN, COLOR_DEFAULT_BG, A_NORMAL),
    srule_new_multi([[\?>]], [[<\?(php)?]], COLOR_WHITE, COLOR_DEFAULT_BG, A_NORMAL)
}

for i, rule in ipairs(php_style) do
    buffer_add_style(buffer_edit, rule)
end
