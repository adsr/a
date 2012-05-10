<?php

/**
 * Syntax highlighting command
 */
class Command_Syntax extends Command {

    public function execute() {

        // Required params
        if (empty($this->params['action'])) {
            Logger::error('Missing action param', __CLASS__);
            return;
        } else if (empty($this->params['syntax_name'])) {
            Logger::error('Missing syntax_name param', __CLASS__);
            return;
        }

        // Resolve BufferView
        $buffer_view = null;
        if ($this->receiver) {
            if (isset($this->params['buffer_num'])) {
                $buffer_view = $this->receiver->getBufferByNumber(
                    (int)$this->params['buffer_num']
                );
            }
            if (!$buffer_view) {
                $buffer_view = $this->receiver->getActiveBufferView();
            }
        }

        // Perform action
        switch ($this->params['action']) {
            case 'add':
                SyntaxHighlighter::addSyntax(
                    $this->params['syntax_name'],
                    $this->params['path_pattern']
                );
                // TODO $this->undo_func = function() { ... };
                break;
            case 'rule':
                SyntaxHighlighter::addSyntaxRule(
                    $this->params['syntax_name'],
                    $this->params['pattern'],
                    $this->params
                );
                break;
            case 'set':
                // Get SyntaxHighlighter
                $highlighter = $buffer_view->getSyntaxHighlighter();
                $highlighter->setSyntax($this->params['syntax_name']);
                break;
        }

    }

    public function undo() {
        // TODO
    }

    public function parseParamString($param_string) {
        $matches = array();
        if (preg_match('/^(\S+)\s+(\S+)\s+(.*)$/', $param_string, $matches)) {
            $this->params['action'] = $matches[1];
            $this->params['syntax_name'] = $matches[2];
            parent::parseParamString($matches[3]);
        }
        Logger::debug(json_encode($this->params), __CLASS__);
    }

}
