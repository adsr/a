local keymap_default = keymap_new(0)
keymap_bind(keymap_default, "left", function(context) mark_move(context.cursor, -1) end)
keymap_bind(keymap_default, "right", function(context) mark_move(context.cursor, 1) end)
keymap_bind(keymap_default, "up", function(context) mark_move_line(context.cursor, -1) end)
keymap_bind(keymap_default, "down", function(context) mark_move_line(context.cursor, 1) end)
keymap_bind(keymap_default, "pageup", function(context) mark_move_line(context.cursor, context.viewport_h * -1) end)
keymap_bind(keymap_default, "pagedown", function(context) mark_move_line(context.cursor, context.viewport_h) end)
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

--[[
keymap_bind(keymap_default, "CR", function(context)
    bview_set_active(bview_prompt)
end)
--]]

keymap_bind_default(keymap_default, function(context)
    local ok, offset, line, col = buffer_insert(context.buffer, context.offset, context.keyc, string.len(context.keyc))
end)



bview_keymap_push(bview_edit, keymap_default)
bview_keymap_push(bview_prompt, keymap_default)
