<?php

/**
 * Represents prompt bar
 */
class Control_PromptBar extends Control { // TODO

    private $window;

    public function resize($rows, $cols, $line, $offset) {
        parent::resize($rows, $cols, $line, $offset);
        if ($this->window) {
            $this->window->delwin();
        }
        $this->window = Ncurses::makeWindow($rows, $cols, $line, $offset);
    }

    public function refresh() {
    }

}
