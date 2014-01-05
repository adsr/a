<?php

/**
 * Absolute cursor movement
 */
class Command_CursorSet extends Command {

    private $buffer_view;

    public function setupReceiver($receiver) {
        $this->buffer_view = $receiver->getActiveBufferView();
    }

    public function execute() {
        $this->buffer_view->setCursor(
            $this->params[0] === '-' ? null : (int)$this->params[0],
            $this->params[1] === '-' ? null : (int)$this->params[1],
            !empty($this->params[2])
        );
    }

    public function undo() {
        // TODO
    }

}
