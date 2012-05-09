<?php

/**
 * Represents status bar
 */
class Control_StatusBar extends Control {

    private $window;
    private $cur_status;
    private $new_status = false;

    public function resize($rows, $cols, $line, $offset) {
        parent::resize($rows, $cols, $line, $offset);
        if ($this->window) {
            $this->window->delwin();
        }
        $this->window = Ncurses::makeWindow($rows, $cols, $line, $offset);
        $this->window->setColor(
            Config::get('status.color_fg', 'black'),
            Config::get('status.color_bg', 'white')
        );
        $this->cur_status = null;
    }

    public function setStatus($str) {
        $this->new_status = $str;
    }

    public function refresh() {
        if ($this->cur_status !== $this->new_status) {
            $this->cur_status = $this->new_status;
            $this->new_status = null;
            $this->window->wmove(0, 0);
            $this->window->waddstr(str_pad($this->cur_status, $this->cols, ' ', STR_PAD_BOTH), $this->cols);
            $this->window->wnoutrefresh();
        }
    }

}
