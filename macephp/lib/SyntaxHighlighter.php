<?php

/**
 * Syntax highlighting class
 */
class SyntaxHighlighter {

    /** @var array */
    private static $rules = array();

    /** @var array */
    private static $syntaxes = array();

    /** @var string */
    private $syntax_name = null;

    /**
     * Add syntax type
     */
    public static function addSyntax($syntax_name, $path_pattern) {
        self::$syntaxes[$syntax_name] = array(
            'path_pattern' => $path_pattern
        );
    }

    /**
     * Add a syntax highlighting rule
     *
     * @param string $syntax_name
     * @param string $pattern regex
     * @param array $style
     */
    public static function addSyntaxRule($syntax_name, $pattern, array $style) {
        if (!isset(self::$rules[$syntax_name])) {
            self::$rules[$syntax_name] = array();
        }
        self::$rules[$syntax_name][] = array($pattern, $style);
    }

    /**
     * Set a certain syntax
     */
    public function setSyntax($syntax_name) {
        $this->syntax_name = $syntax_name;
    }

    /**
     * Try to set syntax by matching path
     */
    public function setSyntaxByPath($path) {
        foreach (self::$syntaxes as $syntax_name => $syntax) {
            if (preg_match($syntax['path_pattern'], $path)) {
                $this->setSyntax($syntax_name);
                return;
            }
        }
    }

    /**
     * Get highlight info for a string of text
     */
    public function highlight($string) {

        if ($this->syntax_name === null || empty(self::$rules[$this->syntax_name])) {
            // No rules
            return;
        } else if (strlen($string) < 1) {
            // Nothing to highlight
            return;
        }

        $rules = self::$rules[$this->syntax_name];
        $attr_map = array_fill(0, strlen($string), null);
        $touched = false;
        foreach ($rules as $rule) {
            list($pattern, $style) = $rule;
            $all_matches = array();
            if (!preg_match_all($pattern, $string, $all_matches, PREG_OFFSET_CAPTURE)) {
                continue;
            }
            foreach ($all_matches as $matches) {
                foreach ($matches as $match) {
                    if (!is_array($match) || count($match) != 2) {
                        continue;
                    }
                    list($substr, $offset) = $match;
                    $this->arraySet(
                        $attr_map,
                        $offset,
                        $offset + strlen($substr) - 1,
                        $this->getNcursesAttrKey($style)
                    );
                    $touched = true;
                }
            }
        }

        if (!$touched) {
            return;
        }

        $colored_substrs = array();
        $current_attr = $attr_map[0];
        $current_offset = 0;
        for ($i = 1, $l = count($attr_map); $i < $l + 1; $i++) {
            if ($i == $l || $attr_map[$i] !== $current_attr) {
                $colored_substrs[] = array(
                    'attr_key' => $current_attr,
                    'offset_start' => $current_offset,
                    'offset_end' => $i - 1,
                    'string' => substr($string, $current_offset, $i - $current_offset)
                );
                if ($i == $l) {
                    break;
                }
                $current_attr = $attr_map[$i];
                $current_offset = $i;
            }
        }

        return $colored_substrs;
    }

    private function getNcursesAttrKey(array $style) {
        return Ncurses::getAttrKey(
            isset($style['color_fg']) ? $style['color_fg'] : 'default',
            isset($style['color_bg']) ? $style['color_bg'] : 'default',
            0
            | (!empty($style['bold']) ? NCURSES_A_BOLD : 0)
            | (!empty($style['reverse']) ? NCURSES_A_REVERSE : 0)
        );
    }

    private function arraySet(&$arr, $start, $end, $value) {
        for ($i = $start; $i <= $end; $i++) {
            $arr[$i] = $value;
        }
    }

}
