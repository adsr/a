<?php

/**
 * Shared context object
 *
 * ContextStack is the slut of the codebase
 */
class ContextStack {

    const ROOT_CONTEXT = 'root';

    /** @var Control */
    private $active_control;

    /** @var Command */
    private $current_command;

    /** @var CommandParser */
    private $command_parser;

    /** @var array */
    private $stack = array();

    /** @var Registry */
    private $registry;

    /** @var Tokenizer */
    private $tokenizer;

    /** Set up some objects */
    public function __construct() {
        $this->registry = new Registry($this);
        $this->tokenizer = new Tokenizer($this->registry);
    }

    /** Sets the currently executing command */
    public function setCurrentCommand(Command $command) {
        $this->current_command = $command;
    }

    /** Gets the current executing command */
    public function getCurrentCommand() {
        return $this->current_command;
    }

    /** Sets the currently focused control */
    public function setActiveControl(Control $control) {
        $this->active_control = $control;
    }

    /** Gets the currently focused control */
    public function getActiveControl() {
        return $this->active_control;
    }

    /** @return Tokenizer */
    public function getTokenizer() {
        return $this->tokenizer;
    }

    /** @param CommandParser $parser */
    public function setCommandParser(CommandParser $parser) {
        $this->command_parser = $parser;
    }

    /** @return CommandParser */
    public function getCommandParser() {
        return $this->command_parser;
    }

    /** @param Tokenizer $tokenizer */
    public function setTokenizer(Tokenizer $tokenizer) {
        $this->tokenizer = $tokenizer;
    }

    /** @return Registry */
    public function getRegistry() {
        return $this->registry;
    }

    /** @param Registry $registry */
    public function setRegistry($registry) {
        $this->registry = $registry;
    }

    /**
     * Add a new context to the stack
     *
     * @param string $context
     */
    public function push($context) {
        $context = (string)$context;
        if (strlen($context) < 1 || preg_match('/[^A-Za-z0-9_-]/', $context)) {
            throw new InvalidArgumentException(
                'Context name should be at least 1 char and consist of ' .
                'letters, numbers, underscores, and dashes only.'
            );
        }
        $this->stack[] = $context;
    }

    /**
     * Pop one context off the stack
     *
     * @return string
     */
    public function pop() {
        if (count($this->stack) < 1) {
            throw new RuntimeException('Already at root context');
        }
        return array_pop($this->stack);
    }

    /** @return int depth of context stack */
    public function getDepth() {
        return count($this->stack);
    }

    /** @return string current context name */
    public function getContextName() {
        $stack_size = count($this->stack);
        return $stack_size > 0 ? implode('.', $this->stack) : self::ROOT_CONTEXT;
    }

}
