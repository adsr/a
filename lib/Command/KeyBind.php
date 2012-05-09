<?php

/**
 * Bind a keychord to a command
 */
class Command_KeyBind extends Command {

    public function execute() {
        $this->context_stack->getCommandParser()->bindKeychordToCommand(
            $this->params['context'],
            $this->params['keychord'],
            $this->params['command']
        );
    }

    public function undo() {
        // TODO?
    }

    public function parseParamString($param_string) {
        parent::parseParamString($param_string);
        if (!isset($this->params['context'])) {
            $this->params['context'] = $this->params[0];
        }
        if (!isset($this->params['keychord'])) {
            $this->params['keychord'] = $this->params[1];
        }
        if (!isset($this->params['command'])) {
            $this->params['command'] = $this->params[2];
        }
    }

}
