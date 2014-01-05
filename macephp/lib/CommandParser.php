<?php

/**
 * Parses a command in the current context
 */
class CommandParser {

    /** @var array { <context name>: { <keychord>: <command>, ... }, ... } */
    private $command_map = array();

    /** @var ContextStack */
    private $context_stack;

    /** @var Tokenizer */
    private $tokenizer;

    /**
     * @param ContextStack $context_stack
     */
    public function __construct(ContextStack $context_stack) {
        $this->context_stack = $context_stack;
        $this->context_stack->setCommandParser($this);
        $this->tokenizer = new Tokenizer($context_stack->getRegistry());
    }

    /**
     * Looks up command to execute for given keychord
     *
     * @param string $keychord
     * @return Command
     */
    public function getCommandFromKeychord($keychord) {
        $context_name = $this->context_stack->getContextName();
        if (!isset($this->command_map[$context_name][$keychord])) {
            throw new CommandParser_Exception(
                "Keychord \\'$keychord\\' not recognized in context \\'$context_name\\'"
            );
        }
        $command_str = $this->command_map[$context_name][$keychord];
        return $this->getCommandFromStr($command_str);
    }

    /**
     * Converts a command string to a Command object
     *
     * @param string $command_str
     * @return Command
     */
    public function getCommandFromStr($command_str) {
        $command_param_str = '';
        $tokenizer_result = $this->tokenizer->tokenize($command_str, 2, false);
        Logger::debug('tokenizer_result=' . json_encode($tokenizer_result), __CLASS__);
        if (!is_array($tokenizer_result) || count($tokenizer_result) > 2
            || count($tokenizer_result) < 1
        ) {
            throw new CommandParser_Exception(
                "Tokenizer expected to return 1 or 2-element array but got " . json_encode($tokenizer_result)
            );
        }
        if (!isset($tokenizer_result[1])) {
            $tokenizer_result[1] = null;
        }
        list($command_name, $command_param_str) = $tokenizer_result;
        $command_class = Command::getClassFromName($command_name);
        if (!class_exists($command_class, true)) {
            throw new CommandParser_Exception(
                "Command class '$command_class' not found"
            );
        }
        $command = new $command_class($this->context_stack);
        $command->parseParamString($command_param_str);
        return $command;
    }

    /**
     * In $context_name, binds $keychord to execute $command_str
     *
     * @param string $context_name
     * @param string $keychord
     * @param string $command_str
     */
    public function bindKeychordToCommand($context_name, $keychord, $command_str) {
        Logger::debug("bindKeychordToCommand($context_name, $keychord, $command_str", __CLASS__);
        $this->command_map[$context_name][$keychord] = $command_str;
    }

}

class CommandParser_Exception extends Exception {
}
