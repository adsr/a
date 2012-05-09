<?php

class Command_StatusSet extends Command {

    public function execute() {
        $this->receiver->setStatus(implode(' ', $this->params));
    }

    public function undo() {
        // TODO?
    }

}
