<?php

/**
 * Move cursor up or down
 */
class Command_CursorMoveLine extends Command {

    private $buffer_view;

    public function setupReceiver($receiver) {
        $this->buffer_view = $receiver->getActiveBufferView();
    }

    public function execute() {
        $this->buffer_view->moveCursorLine((int)$this->params[0]);
    }

    public function undo() {
        // TODO
    }

}
