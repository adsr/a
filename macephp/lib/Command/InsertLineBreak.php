<?php

/**
 * Insert line break
 */
class Command_InsertLineBreak extends Command {

    private $buffer;
    private $buffer_view;

    public function setupReceiver($receiver) {
        $this->buffer_view = $receiver->getActiveBufferView();
        $this->buffer = $this->buffer_view->getBuffer();
    }

    public function execute() {
        list($cur_line, $cur_offset, ) = $this->buffer->spliceString(
            $this->params['line'],
            $this->params['offset'],
            "\n",
            0
        );
        $this->buffer_view->setCursor($cur_line, $cur_offset);
    }

    public function undo() {
        // TODO
    }

    public function parseParamString($param_string) {
        parent::parseParamString($param_string);
        if (!isset($this->params['line'])) {
            $this->params['line'] = $this->buffer_view->getCursorLine();
        }
        if (!isset($this->params['offset'])) {
            $this->params['offset'] = $this->buffer_view->getCursorOffset();
        }
    }

}
