<?php

/**
 * Syntax highlighting class
 */
class SyntaxHighlighter { // TODO

    public function highlight($string) {

        if (strlen($string) < 1) {
            return;
        }

        $color_patterns = array(
            array('cyan', '/\d+/'),
            array('green', '/adam/'),
            array('red', '/[A-Z]+/')
        );
        $matched_substrs = array();
        foreach ($color_patterns as $color_pattern) {
            list($color_fg, $pattern) = $color_pattern;
            $all_matches = array();
            if (!preg_match_all($pattern, $string, $all_matches, PREG_OFFSET_CAPTURE)) {
                continue;
            }
            foreach ($all_matches as $matches) {
                foreach ($matches as $match) {
                    list($substr, $offset) = $match;
                    $matched_substrs[] = array(
                        'color_fg' => $color_fg,
                        'color_bg' => 'default',
                        'string' => $substr,
                        'offset_start' => $offset,
                        'offset_end' => $offset + strlen($substr) - 1
                    );
                }
            }
        }
        $color_map = array();
        for ($i = 0, $l = strlen($string); $i < $l; $i++) {
            $color_fg = null;
            foreach ($matched_substrs as $matched_substr) {
                if ($i >= $matched_substr['offset_start']
                    && $i <= $matched_substr['offset_end']
                ) {
                    $color_fg = $matched_substr['color_fg'];
                    break;
                }
            }
            if (!$color_fg) {
                $color_fg = 'default';
            }
            $color_map[$i] = $color_fg;
        }
        $colored_substrs = array();
        $current_color = $color_map[0];
        $current_offset = 0;
        for ($i = 1, $l = count($color_map); $i < $l + 1; $i++) {
            if ($i == $l || $color_map[$i] != $current_color) {
                $colored_substrs[] = array(
                    'color_fg' => $current_color,
                    'color_bg' => 'default',
                    'offset_start' => $current_offset,
                    'offset_end' => $i - 1,
                    'string' => substr($string, $current_offset, $i - $current_offset)
                );
                if ($i == $l) {
                    break;
                }
                $current_color = $color_map[$i];
                $current_offset = $i;
            }
        }

        return $colored_substrs;
    }

}
