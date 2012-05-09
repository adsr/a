<?php

/**
 * Insert tab
 */
class Command_InsertTab extends Command_InsertString {

    public function parseParamString($param_string) {
        parent::parseParamString($param_string);
        $tab_stop = Config::get('tab_stop', 4);
        $cur_offset = $this->buffer_view->getCursorOffset();
        $num_spaces_to_tab_stop = 4 - ($cur_offset % $tab_stop);
        $this->params['str'] = str_repeat(' ', $num_spaces_to_tab_stop);
    }

}
