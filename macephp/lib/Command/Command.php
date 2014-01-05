<?php

abstract class Command {
    protected $params;
    protected $receiver;
    public function setReceiver($receiver) {
        $this->receiver = $receiver;
    }
    public function setParams(array $params) {
        $this->params = $params;
    }
    abstract public function execute();
    abstract public function undo();
}