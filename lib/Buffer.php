<?php

/**
 * ASCII string-backed implementation of IBuffer
 */
class Buffer implements IBuffer {

    /** @var array */
    private $buffer_listeners = array();

    /** @var string */
    private $buffer = '';

    /** @var int */
    private $buffer_len = 0;

    /** @var array { <line_num>: <byte_offset>, ... } */
    private $line_offsets = array(0 => 0);

    /**
     * Registers a buffer listener
     *
     * @param IBufferListener $bl
     */
    public function registerListener(IBufferListener $bl) {
        $bl_hash = spl_object_hash($bl);
        if (!isset($this->buffer_listeners[$bl_hash])) {
            $this->buffer_listeners[$bl_hash] = $bl;
        }
    }

    /**
     * Loads buffer from string
     *
     * @param string $text
     */
    public function loadFromText($text) {
        $this->buffer = $this->normalizeString($text);
        $this->buffer_len = strlen($this->buffer);
        $this->calculateLineOffsets();
    }

    /** @return string */
    public function __toString() {
        return $this->buffer;
    }

    /** @return int */
    public function getLineCount() {
        return count($this->line_offsets);
    }

    /** @return int */
    public function getCharCount() {
        return $this->buffer_len;
    }

    /**
     * @param int $buffer_offset
     * @return array [<line>, <column>]
     */
    public function getLineAndOffset($buffer_offset) {
        if ($buffer_offset < 1) {
            return array(0, 0);
        }
        $prev_start_offset = 0;
        foreach ($this->line_offsets as $line => $start_offset) {
            if ($start_offset == $buffer_offset) {
                return array($line, 0);
            }
            else if ($start_offset > $buffer_offset) {
                return array(
                    max(0, $line - 1),
                    max(0, $buffer_offset - $prev_start_offset)
                );
            }
            $prev_start_offset = $start_offset;
        }
        $last_line = $this->getLineCount() - 1;
        return array(
            max(0, $this->getLineCount() - 1),
            max(0,
                min(
                    $buffer_offset - $prev_start_offset,
                    $this->buffer_len - $this->line_offsets[$last_line]
                )
            )
        );
    }

    /**
     * @param int $line
     * @param int $offset
     * @return int buffer offset
     */
    public function getBufferOffset($line, $offset) {
        if ($line < 0) {
            return 0;
        }
        return max(
            0,
            isset($this->line_offsets[$line])
                ? $this->line_offsets[$line] + $offset
                : $this->buffer_len
        );
    }

    /**
     * Inserts and deletes characters from buffer and returns where cursor
     * should move to
     *
     * @param int $line
     * @param int $offset
     * @param string $insert_str
     * @param int $chars_to_delete
     * @return array [<line>, <column, <buffer offset>]
     */
    public function spliceString($line, $offset, $insert_str, $chars_to_delete) {
        Logger::debug(
            'spliceString: line=%d offset=%d insert_str=%s chars_to_delete=%d',
            __CLASS__,
            $line, $offset, $insert_str, $chars_to_delete
        );

        $pre_line_count = $this->getLineCount();

        $insert_str = $this->normalizeString($insert_str);
        $chars_to_insert = strlen($insert_str);

        if (!is_int($chars_to_delete)) {
            $chars_to_delete = strlen((string)$chars_to_delete);
        }

        $insert_offset = $this->getBufferOffset($line, $offset);

        $this->buffer = substr($this->buffer, 0, $insert_offset)
            . $insert_str
            . substr($this->buffer, $insert_offset + $chars_to_delete);
        $this->buffer_len = max(0, $this->buffer_len + $chars_to_insert - $chars_to_delete);

        $this->calculateLineOffsets();

        $new_insert_offset = max(0, $insert_offset + $chars_to_insert);
        list($new_line, $new_offset) = $this->getLineAndOffset($new_insert_offset);

        $post_line_count = $this->getLineCount();
        if ($post_line_count == $pre_line_count) {
            $this->dirtyLines(min($line, $new_line), max($line, $new_line), false);
        }
        else if ($post_line_count > $pre_line_count) {
            $this->dirtyLines(min($line, $new_line), $post_line_count - 1, false);
        }
        else { // $post_line_count < $pre_line_count
            $this->dirtyLines(min($line, $new_line), $post_line_count - 1, true);
        }

        return array(
            $new_line,
            $new_offset,
            $new_insert_offset
        );
    }

    /**
     * @param int $start_line
     * @param int $end_line
     * @return array { <line>: <line contents>, ... }
     */
    public function getLines($start_line, $end_line) {
        $lines = array();
        for ($line = $start_line; $line <= $end_line; $line++) {
            $lines[$line] = $this->getLine($line);
        }
        return $lines;
    }

    /**
     * @param int $line
     * @return string
     */
    private function getLine($line) {
        $line_count = $this->getLineCount();
        if ($line >= $line_count) {
            return null;
        }
        $start_offset = $this->getBufferOffset($line, 0);
        if ($line + 1 < $line_count) {
            $end_offset = $this->getBufferOffset($line + 1, 0) - 1; // minus newline
        }
        else {
            $end_offset = $this->buffer_len;
        }
        $line_str = substr($this->buffer, $start_offset, $end_offset - $start_offset);
        return $line_str;
    }

    /**
     * @param int $line
     * @return int
     */
    public function getLineLength($line) {
        if (!isset($this->line_offsets[$line])) {
            return null;
        }
        $next_line = $line + 1;
        if (isset($this->line_offsets[$next_line])) {
            return $this->line_offsets[$next_line]
                 - $this->line_offsets[$line]
                 - 1;
        }
        else {
            return $this->buffer_len
                 - $this->line_offsets[$line];
        }
    }

    /** Recalculate $this->line_offsets */
    private function calculateLineOffsets() {
        $line_offsets = array(0 => 0);
        $look_offset = 0;
        $newline_offset = 0;
        $line = 1;
        while (true) {
            $newline_offset = strpos($this->buffer, "\n", $look_offset);
            if ($newline_offset === false) {
                break;
            }
            $line_offsets[$line] = $newline_offset + 1;
            $look_offset = $line_offsets[$line];
            $line += 1;
        }
        $this->line_offsets = $line_offsets;
    }

    /**
     * @param $str
     * @return ASCII version of $str with unix style line endings
     */
    private function normalizeString($str) {
        return preg_replace(
            '/(?:\r\n|\n|\r)/',
            "\n",
            iconv('UTF-8', 'ASCII//TRANSLIT', (string)$str)
        );
    }

    /**
     * Mark $start_line to $end_line as dirty (changed)
     *
     * @param int $start_line
     * @param int $end_line
     * @param bool $line_count_decreased
     */
    private function dirtyLines($start_line, $end_line, $line_count_decreased) {
        foreach ($this->buffer_listeners as $buffer_listener) {
            $buffer_listener->onDirtyLines($start_line, $end_line, $line_count_decreased);
        }
    }

}
