<?php

/**
 * Variable-aware string tokenizing class
 */
class Tokenizer {

    private $registry;
    private $use_registry;
    private $tokens;
    private $cur_token;
    private $next_token_is_variable;

    const REGEX_WHITESPACE = '|^\s+$|';

    public function __construct($registry = null) {
        $this->registry = $registry instanceof Registry ? $registry : null;
    }

    public function tokenize($str, $max = -1, $use_registry = true) {

        $this->tokens = array();
        $this->cur_token = '';
        $this->next_token_is_variable = false;
        $this->use_registry = (bool)$this->registry && (bool)$use_registry;

        $len = strlen($str);

        $next_char_is_literal = false;
        $quote_char = null;

        for ($i = 0; $i < $len; $i++) {
            $is_last_char = ($i === $len - 1);
            $char = substr($str, $i, 1);
            $is_char_whitespace = preg_match(self::REGEX_WHITESPACE, $char);
            if ($next_char_is_literal) {
                // Literal character
                $this->appendToCurrentToken($char, true);
                $next_char_is_literal = false;
            }
            else if (!$quote_char
                && $is_char_whitespace
                && strlen($this->cur_token) > 0
            ) {
                // Add token to token list
                $this->addToken();
                if (count($this->tokens) === $max && !$is_last_char) {
                    $this->cur_token = $char . substr($str, $i + 1);
                    $this->addToken($max - 1);
                    break;
                }
            }
            else if ($char === $quote_char) {
                // End quoted token
                $quote_char = null;
            }
            else if ($char === '\\') {
                // Escape next char
                $next_char_is_literal = true;
            }
            else if (($char === "'" && $quote_char !== '"') || ($char === '"' && $quote_char !== "'")) {
                // Enter quoted token
                $quote_char = $char;
            }
            else if ($quote_char || !$is_char_whitespace) {
                // Append char to token
                $this->appendToCurrentToken($char);
            }
        }
        if (strlen($this->cur_token) > 0) {
            // Add left over token to token list
            $this->addToken();
        }
        return $this->tokens;

    }

    private function addToken($append_to_key = null) {
        if ($append_to_key !== null) {
            $this->tokens[$append_to_key] .= $this->cur_token;
        }
        else {
            if ($this->next_token_is_variable && strlen($this->cur_token) > 1) {
                $this->cur_token = $this->registry->evaluate($this->cur_token);
                $this->next_token_is_variable = false;
            }
            $this->tokens[] = $this->cur_token;
        }
        $this->cur_token = '';
    }

    private function appendToCurrentToken($char, $force_literal = false) {
        $this->cur_token .= $char;
        if (!$force_literal
            && strlen($this->cur_token) == 1
            && $this->use_registry
            && $this->registry->isVarPrefix($this->cur_token)
        ) {
            $this->next_token_is_variable = true;
        }
    }

}
