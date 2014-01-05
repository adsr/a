<?php

/**
 * Represents title bar
 */
class Control_TitleBar extends Control {

    private $window;
    private $title;

    public function resize($rows, $cols, $line, $offset) {
        parent::resize($rows, $cols, $line, $offset);
        if ($this->window) {
            $this->window->delwin();
        }
        $this->window = Ncurses::makeWindow($rows, $cols, $line, $offset);
        $this->window->setColor(
            Config::get('title.color_fg', 'black'),
            Config::get('title.color_bg', 'white')
        );
        $this->title = null;
    }

    public function refresh() {
        $new_title = Config::get('title.text', 'mace ' . MACE_VERSION);
        // TODO callable
        if ($this->title !== $new_title) {
            $this->title = $new_title;
            $this->window->wmove(0, 0);
            $this->window->waddstr(str_pad($this->title, $this->cols, ' ', STR_PAD_BOTH), $this->cols);
            $this->window->wnoutrefresh();
        }
    }

}
