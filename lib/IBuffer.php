<?php

interface IBuffer {
    public function registerListener(IBufferListener $listener);
    public function loadFromText($text);
    public function __toString();
    public function getLineCount();
    public function getCharCount();
    public function getLineAndOffset($buffer_offset);
    public function getBufferOffset($line, $offset);
    public function spliceString($line, $offset, $insert_str, $chars_to_delete);
    public function getLines($start_line, $end_line);
}
