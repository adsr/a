local keymap_default = keymap_new(0)
keymap_bind(keymap_default, "left", function(context) mark_move(context.cursor, -1) end)
keymap_bind(keymap_default, "right", function(context) mark_move(context.cursor, 1) end)
keymap_bind(keymap_default, "up", function(context) mark_move_line(context.cursor, -1) end)
keymap_bind(keymap_default, "down", function(context) mark_move_line(context.cursor, 1) end)
keymap_bind(keymap_default, "home", function(context) mark_set(context.line, 0) end)
keymap_bind(keymap_default, "CA", function(context) mark_set(context.line, 0) end)
keymap_bind(keymap_default, "pageup", function(context) mark_move_line(context.cursor, context.viewport_h * -1) end)
keymap_bind(keymap_default, "pagedown", function(context) mark_move_line(context.cursor, context.viewport_h) end)
--- keymap_bind(keymap_default, "end", function(context) mark_set(context.line, bline_get_length(context.bline) - 1) end)
--- keymap_bind(keymap_default, "CE", function(context) mark_set(context.line, bline_get_length(context.bline) - 1) end)
keymap_bind(keymap_default, "backspace", function(context)
    if context.offset > 0 then
        buffer_delete(context.buffer, context.offset - 1, 1)
    end
end)
keymap_bind(keymap_default, "delete", function(context)
    if context.offset < context.byte_count then
        ok, line, col = mark_get(context.cursor)
        buffer_delete(context.buffer, context.offset, 1)
        mark_set_line_col(context.cursor, line, col)
    end
end)
keymap_bind(keymap_default, "enter", function(context)
    ok, offset, line, col = buffer_insert(context.buffer, context.offset, "\n", 1)
end)

--[[
keymap_bind(keymap_default, "CR", function(context)
    bview_set_active(bview_prompt)
end)
--]]

keymap_bind_default(keymap_default, function(context)
    ok, offset, line, col = buffer_insert(context.buffer, context.offset, context.keyc, string.len(context.keyc))
end)



bview_keymap_push(bview_edit, keymap_default)
bview_keymap_push(bview_prompt, keymap_default)
