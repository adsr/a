<?php

/**
 * Backspace command
 */
class Command_DeleteCharBefore extends Command {

    private $buffer;
    private $buffer_view;

    public function setupReceiver($receiver) {
        $this->buffer_view = $receiver->getActiveBufferView();
        $this->buffer = $this->buffer_view->getBuffer();
    }

    public function execute() {
        if ($this->params['line'] < 1 && $this->params['offset'] < 1) {
            return;
        }
        list($cur_line, $cur_offset, ) = $this->buffer->spliceString(
            $this->params['line'],
            $this->params['offset'] - 1,
            '',
            1
        );
        $this->buffer_view->setCursor(
            $cur_line,
            $cur_offset
        );
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
