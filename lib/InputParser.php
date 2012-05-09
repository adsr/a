<?php

/**
 * Class for figuring out keychords from raw client input
 * Mainly esoteric bs
 */
class InputParser {

    private $func_get_input;

    private $override_code_map = array(
        '8' => 'backspace',
        '9' => 'tab',
        '28' => 'ctrl-f4',
        '29' => 'ctrl-f5',
        '30' => 'ctrl-f6',
        '31' => 'ctrl-f7',
        '32' => 'space',
        '13' => 'enter',
        '34' => 'double-quote',
        '39' => 'single-quote',
        '127' => 'backspace',
        '258' => 'down',
        '259' => 'up',
        '260' => 'left',
        '261' => 'right',
        '262' => 'home',
        '263' => 'backspace',
        '330' => 'delete',
        '331' => 'insert',
        '338' => 'page-down',
        '339' => 'page-up',
        '343' => 'enter',
        '350' => '5',
        '360' => 'end',
        '410' => 'resize',
    );

    private $escape_code_map = array(
        '[A' => 'up',
        '[B' => 'down',
        '[C' => 'right',
        '[D' => 'left',
        '[E' => '5',
        '[F' => 'end',
        '[G' => '5',
        '[H' => 'home',
        '[M' => 'mouse',
        '[1~' => 'home',
        '[2~' => 'insert',
        '[3~' => 'delete',
        '[4~' => 'end',
        '[5~' => 'page-up',
        '[6~' => 'page-down',
        '[7~' => 'home',
        '[8~' => 'end',
        '[[A' => 'f1',
        '[[B' => 'f2',
        '[[C' => 'f3',
        '[[D' => 'f4',
        '[[E' => 'f5',
        '[11~' => 'f1',
        '[12~' => 'f2',
        '[13~' => 'f3',
        '[14~' => 'f4',
        '[15~' => 'f5',
        '[17~' => 'f6',
        '[18~' => 'f7',
        '[19~' => 'f8',
        '[20~' => 'f9',
        '[21~' => 'f10',
        '[23~' => 'f11',
        '[24~' => 'f12',
        '[25~' => 'f13',
        '[26~' => 'f14',
        '[28~' => 'f15',
        '[29~' => 'f16',
        '[31~' => 'f17',
        '[32~' => 'f18',
        '[33~' => 'f19',
        '[34~' => 'f20',
        'OA' => 'up',
        'OB' => 'down',
        'OC' => 'right',
        'OD' => 'left',
        'OH' => 'home',
        'OF' => 'end',
        'OP' => 'f1',
        'OQ' => 'f2',
        'OR' => 'f3',
        'OS' => 'f4',
        'Oo' => '/',
        'Oj' => '*',
        'Om' => '-',
        'Ok' => '+',
        '[Z' => 'shift-tab',
    );

    private $code_trie = array();

    public function __construct($func_get_input) {
        $this->func_get_input = $func_get_input;
        $this->buildKeyCodeTrie();
    }

    public function getKeyChord() {
        $trie_node = &$this->code_trie;
        do {
            $code = call_user_func($this->func_get_input);
            if (!isset($trie_node[$code])) {
                return null;
            }
            $trie_node = &$trie_node[$code];
        } while (is_array($trie_node));
        if ($trie_node === 'mouse') {
            $this->readMouseInfo();
        }
        return (string)$trie_node;
    }

    public function getLastMouseInfo() {
        return $this->mouse_info;
    }

    private function readMouseInfo() {

        $mouse_code = array();
        for ($i < 0; $i < 3; $i++) {
            $mouse_code[$i] = call_user_func($this->func_get_input);
        }

        $modifier = $mouse_code[0] - 32;
        $x = $mouse_code[1] - 33;
        $y = $mouse_code[2] - 33;

        $button = (($modifier & 64) / 64 * 3) + ($modifier & 3) + 1;

        $action = 'down';
        if ($modifier & 3) {
            $action = 'up';
            $button = 0;
        }
        else if ($modifier & 2048) {
            $action = 'up';
        }
        else if ($modifier & 32) {
            $action = 'drag';
        }

        $this->mouse_info = array(
            'action' => $action,
            'x' => $x,
            'y' => $y,
            'button' => $button,
            'shift' => $modifier & 4 ? true : false,
            'alt' => $modifier & 8 ? true : false,
            'ctrl' => $modifier & 16 ? true : false,
        );
    }

    private function buildKeyCodeTrie() {
        // Build trie
        $trie = &$this->code_trie;

        // ctrl a-z
        for ($code = 1; $code <= 26; $code++) {
            $trie[$code] = 'ctrl-' . chr(ord('a') + $code - 1);
        }

        // printable ASCII
        for ($code = 33; $code <= 126; $code++) {
            $trie[$code] = chr($code);
        }

        // f1-f12
        for ($code = 265; $code <= 276; $code++) {
            $trie[$code] = 'f' . ($code - 264);
        }

        // shift f1-f12
        for ($code = 277; $code <= 288; $code++) {
            $trie[$code] = 'shift-f' . ($code - 276);
        }

        // escape codes
        $trie['27'] = array();
        $trie['27']['32'] = 'alt-space';
        for ($code = 33; $code <= 126; $code++) {
            $trie['27'][$code] = 'alt-' . chr($code);
        }
        foreach ($this->escape_code_map as $escape_chord => $chord_name) {
            $trie_node = &$trie['27'];
            $escape_chars = str_split($escape_chord);
            for ($i = 0, $l = count($escape_chars); $i < $l; $i++) {
                $code = ord($escape_chars[$i]);
                if ($i == $l - 1) {
                    $trie_node[$code] = $chord_name;
                }
                else if (!isset($trie_node[$code]) || !is_array($trie_node[$code])) {
                    $trie_node[$code] = array();
                }
                $trie_node = &$trie_node[$code];
            }
        }

        // override_code_map
        foreach ($this->override_code_map as $code => $chord) {
            $trie[$code] = $chord;
        }

    }

}
