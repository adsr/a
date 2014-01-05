<?php

/**
 * Relative left-and-right cursor movement
 */
class Command_CursorMoveOffset extends Command {

    private $buffer_view;

    public function setupReceiver($receiver) {
        $this->buffer_view = $receiver->getActiveBufferView();
    }

    public function execute() {
        $this->buffer_view->moveCursorOffset((int)$this->params[0]);
    }

    public function undo() {
        // TODO
    }

}
