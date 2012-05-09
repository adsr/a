<?php

/**
 * Control that represents a buffer on screen
 */
class Control_BufferView extends Control implements IBufferListener {

    /** @var IBuffer */
    private $buffer;

    /** @var SyntaxHighlighter */
    private $syntax_highlighter;

    /* @var int */
    private $window_line_num;

    /* @var int */
    private $window_left_margin;

    /* @var int */
    private $window_buffer;

    /* @var int */
    private $window_right_margin;

    /* @var int */
    private $buffer_cols;

    /* @var int */
    private $line_num_cols;

    /* @var int */
    private $cursor_line = 0;

    /* @var int */
    private $cursor_offset = 0;

    /* @var int */
    private $target_cursor_offset = 0;

    /* @var int */
    private $viewport_line = 0;

    /* @var int */
    private $viewport_line_end = 0;

    /* @var int */
    private $viewport_offset = 0;

    /* @var bool */
    private $first_refresh = true;

    /* @var bool */
    private $has_focus = true;

    /** Ctor */
    public function __construct($buffer = null) {
        if ($buffer instanceof IBuffer) {
            $this->buffer = $buffer;
        }
        else if (is_string($buffer)) {
            // TODO load from path
        }
        else {
            $this->buffer = new Buffer();
            $this->buffer->loadFromText(file_get_contents(__DIR__ . '/../Mace.php')); // TODO don't do this
        }
        $this->syntax_highlighter = new SyntaxHighlighter(); // TODO setSyntaxHighlighter
        $this->buffer->registerListener($this);
        $this->line_num_cols = Config::get('buffer_view.line_number_width', 4);
    }

    /** Set focus */
    public function setFocus($on_off) {
        $this->has_focus = (bool)$on_off;
    }

    /** Clone */
    public function __clone() {
        $this->window_line_num = null;
        $this->window_left_margin = null;
        $this->window_buffer = null;
        $this->window_right_margin = null;
        $this->first_refresh = true;
        $this->resize($this->rows, $this->cols, $this->line, $this->offset);
        $this->buffer->registerListener($this);
    }

    /** Return minimum space required to display */
    public function getMinimumDimensions() {
        return array(
            1,
            $this->line_num_cols + 1 + 1 + 1
        );
    }

    /** @return IBuffer */
    public function getBuffer() {
        return $this->buffer;
    }

    /**
     * Resize buffer view according to params
     */
    public function resize($rows, $cols, $line, $offset) {

        parent::resize($rows, $cols, $line, $offset);
        // TODO if smaller than getMinimumDimensions, throw exception

        if ($this->window_line_num) {
            $this->window_line_num->delwin();
            $this->window_left_margin->delwin();
            $this->window_buffer->delwin();
            $this->window_right_margin->delwin();
        }

        $line_num_width = $this->line_num_cols;
        $this->buffer_cols = $cols - $line_num_width - 1 - 1;

        $this->window_line_num = Ncurses::makeWindow(
            $rows,
            $line_num_width,
            $line,
            $offset
        );
        $this->window_line_num->setColor(
            Config::get('buffer_view.line_num.color_fg', 'yellow'),
            Config::get('buffer_view.line_num.color_bg', 'default')
        );

        $this->window_left_margin = Ncurses::makeWindow(
            $rows,
            1,
            $line,
            $offset + $line_num_width
        );

        $this->window_buffer = Ncurses::makeWindow(
            $rows,
            $this->buffer_cols,
            $line,
            $offset + $line_num_width + 1
        );

        $this->window_right_margin = Ncurses::makeWindow(
            $rows,
            1,
            $line,
            $offset + $line_num_width + 1 + $this->buffer_cols
        );

        $this->viewport_line_end = $this->viewport_line + $rows - 1;

        $this->first_refresh = true;

    }

    /**
     * Refresh control
     */
    public function refresh() {
        if ($this->first_refresh) {
            $this->onDirtyLines(0, $this->rows - 1, false);
            $this->clearLinesAfter($this->buffer->getLineCount());
            $this->first_refresh = false;
        }
        if ($this->registry && $this->has_focus) {
            $this->registry->setVar('cur_line_num', $this->cursor_line);
            $this->registry->setVar('cur_line_length', $this->buffer->getLineLength($this->cursor_line));
        }
    }

    /**
     * Called by IBuffer when lines are edited
     */
    public function onDirtyLines($dirty_line_start, $dirty_line_end, $line_count_decreased) {
        $display_line_start = $this->viewport_line;
        $display_line_end = $this->viewport_line_end;
        if ($dirty_line_start > $display_line_end
            || $dirty_line_end < $display_line_start
        ) {
            return;
        }
        $update_line_start = max($display_line_start, $dirty_line_start);
        $update_line_end = min($display_line_end, $dirty_line_end);
        $lines = $this->buffer->getLines($update_line_start, $update_line_end);
        foreach ($lines as $line_index => $line_content) {
            $this->renderLine($line_content, $line_index - $display_line_start, $line_index);
        }
        if ($line_count_decreased) {
            $this->clearLinesAfter($update_line_end + 1);
        }
        $this->window_line_num->wnoutrefresh();
        $this->window_buffer->wnoutrefresh();
    }

    /**
     * Render a line on screen
     */
    public function renderLine($line_content, $screen_line, $line_index) {

        // TODO happens in split mode.. not sure why
        if (!$this->window_buffer) {
            try { throw new Exception(json_encode($this->window_buffer)); } catch (Exception $e) {
                ncurses_end();
                echo $e;
                die();
            }
        }

        $screen_offset = 0;
        if ($line_content !== null) {
            $colored_substrs = $this->syntax_highlighter->highlight($line_content);
            if ($colored_substrs) {
                foreach ($colored_substrs as $colored_substr) {
                    if ($colored_substr['offset_end'] < $this->viewport_offset
                        || $colored_substr['offset_start'] > $this->viewport_offset + $this->buffer_cols) {
                        continue;
                    }
                    $offset_end = min($colored_substr['offset_end'], $this->viewport_offset + $this->buffer_cols);
                    $offset_start = max($colored_substr['offset_start'], $this->viewport_offset);
                    $substr = substr(
                        $colored_substr['string'],
                        0,
                        $offset_end - $offset_start + 1
                    );
                    $this->window_buffer->setColor(
                        $colored_substr['color_fg'],
                        $colored_substr['color_bg']
                    );
                    $this->window_buffer->wmove($screen_line, $screen_offset);
                    $this->window_buffer->waddstr($substr);
                    $screen_offset += strlen($substr);
                }
            }
            else {
                $this->window_buffer->wmove($screen_line, $screen_offset);
                $this->window_buffer->setColor('default', 'default');
                $this->window_buffer->waddstr(str_pad(substr($line_content, $this->viewport_offset, $this->buffer_cols), $this->buffer_cols));
                $screen_offset = $this->buffer_cols;
            }
        }
        if ($screen_offset < $this->buffer_cols) {
            $this->window_buffer->wmove($screen_line, $screen_offset);
            $this->window_buffer->setColor('default', 'default');
            $this->window_buffer->waddstr(str_repeat(' ', $this->buffer_cols - $screen_offset));
        }
        //$this->window_buffer->wcolor_set(Ncurses::getColorPair('yellow', 'green'));
        //$this->window_buffer->wmove($screen_line, 0);
        //$this->window_buffer->waddstr(str_pad(substr($line_content, $this->viewport_offset, $this->buffer_cols), $this->buffer_cols));
        $this->window_line_num->wmove($screen_line, 0);
        $this->window_line_num->waddstr(sprintf("% {$this->line_num_cols}s", $line_content === null ? '~' : $line_index + 1));
    }

    /**
     * Clear buffer lines after $clear_line_start
     */
    private function clearLinesAfter($clear_line_start) {
        for ($line_to_clear = max($clear_line_start, $this->viewport_line);
             $line_to_clear <= $this->viewport_line_end;
             $line_to_clear++
        ) {
            $this->renderLine(null, $line_to_clear - $this->viewport_line, $line_to_clear);
        }
    }

    /** @return int */
    public function getCursorLine() {
        return $this->cursor_line;
    }

    /** @return int */
    public function getCursorOffset() {
        return $this->cursor_offset;
    }

    /** @param int $line_delta */
    public function moveCursorLine($line_delta) {
        $this->setCursor(
            $this->cursor_line + $line_delta,
            0
        );
        $this->setCursor(
            $this->cursor_line,
            min(
                $this->buffer->getLineLength($this->cursor_line),
                $this->target_cursor_offset
            )
        );
    }

    /** @param int $offset_delta */
    public function moveCursorOffset($offset_delta) {
        $this->setCursor(
            $this->cursor_line,
            $this->cursor_offset + $offset_delta
        );
        $this->target_cursor_offset = $this->cursor_offset;
    }

    /**
     * @param int $set_line
     * @param int $set_offset
     */
    public function setCursor($set_line, $set_offset) {
        list($line, $offset) = $this->buffer->getLineAndOffset(
            $this->buffer->getBufferOffset(
                $set_line,
                $set_offset
            )
        );
        $this->cursor_line = $line;
        $this->cursor_offset = $offset;

        if ($this->cursor_line > $this->viewport_line_end) {
            $this->moveViewportLine($this->cursor_line - $this->viewport_line_end);
        }
        else if ($this->cursor_line < $this->viewport_line) {
            $this->moveViewportLine(-1 * ($this->viewport_line - $this->cursor_line));
        }

    }

    /** @param int $delta */
    public function moveViewportLine($delta) {
        $this->setViewportLine($this->viewport_line + $delta);
    }

    /** @param int $line */
    public function setViewportLine($line) {
        if ($this->viewport_line == $line) {
            return;
        }
        $this->viewport_line = $line;
        $this->viewport_line_end = $line + $this->rows - 1;
        $this->onDirtyLines($this->viewport_line, $this->viewport_line_end, false);
    }

    /** @return array cursor state like [<line>, <offset>, <visibility flag>] */
    public function getCursorState() {
        return array(
            $this->window_buffer->line + $this->cursor_line - $this->viewport_line,
            $this->window_buffer->offset + $this->cursor_offset,
            2
        );
    }

}
