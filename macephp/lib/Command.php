<?php

/**
 * All concrete commands extend this class
 */
abstract class Command {

    /** @staticvar object sentinel value that a command returns to signify quit */
    public static $RESULT_HALT = null;

    /** @const string return values */
    const LAST_RESULT_VAR_NAME = '_';

    /** @var array command params */
    protected $params = array();

    /** @var string unparsed param string */
    protected $param_str;

    /** @var object target of command */
    protected $receiver;

    /** @var Tokenzier */
    protected $tokenizer;

    /** @var ContextStack */
    protected $context_stack;

    /**
     * @param ContextStack $stack
     * @param $receiver defaults to active control if null
     */
    public function __construct(ContextStack $stack, $receiver = null) {
        $this->tokenizer = $stack->getTokenizer();
        $this->context_stack = $stack;
        $this->receiver = $receiver ?: $stack->getActiveControl();
        $this->setupReceiver($this->receiver);
    }

    /** Do pre-command setup on receiver */
    public function setupReceiver($receiver) {
        // No-op by default
    }

    /** @return bool */
    protected function useRegistry() {
        return true;
    }

    /** Execute the command */
    abstract public function execute();

    /** Undo the command */
    abstract public function undo();

    /**
     * Parse the param string. The default implementation converts switch-like
     * params (-foo and --bar) to named params
     *
     * @param string $param_string
     */
    public function parseParamString($param_string) {
        $this->param_str = $param_string;
        $params = array();
        $tokens = $this->tokenizer->tokenize($param_string, -1, $this->useRegistry());
        Logger::debug('parseParamString: tokens=' . json_encode($tokens), __CLASS__);
        $next_token_is_rvalue = false;
        $next_token_param_key = null;
        $match = array();
        foreach ($tokens as $i => $token) {
            if ($next_token_is_rvalue) {
                $params[$next_token_param_key] = $token;
                $next_token_is_rvalue = false;
            }
            else if (preg_match('|^--?([A-Za-z]\w*)=(.*)$|', $token, $match)) {
                $params[$match[1]] = $match[2];
            }
            else if (preg_match('|^--?([A-Za-z]\w*)$|', $token, $match)) {
                $next_token_is_rvalue = true;
                $next_token_param_key = $match[1];
            }
            else {
                $params[$i] = $token;
            }
        }
        Logger::debug('parseParamString: params=' . json_encode($params), __CLASS__);
        $this->params = array_merge($this->params, $params);
    }

    /**
     * @return string lowercase-with-underscores version of command name
     */
    final public function getCommandName() {
        $command_camel = substr(get_class($this), 8); // Just the substring after 'Command_'
        return strtolower(preg_replace('|[a-z]([A-Z])|e', '\'_\' . strtolower(\'$1\')', $command_camel));
    }

    /**
     * @return string Command class name given lowercase-with-underscores version
     */
    public static function getClassFromName($command_name) {
        return 'Command_' . preg_replace(array('|^(.)|e', '|_(.)|e'), 'strtoupper(\'$1\')', $command_name);
    }

    /** @return string command string */
    final public function __toString() {
        return $this->getCommandName() . ' ' . $this->param_str;
    }

}

Command::$RESULT_HALT = new stdClass(); // Set sentinel value
