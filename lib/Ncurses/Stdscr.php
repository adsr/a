<?php

class Ncurses_Stdscr extends Ncurses {

    private static $instance;

    public static function getInstance() {
        if (!self::$instance) {
            if (!defined('STDSCR')) {
                ncurses_init();
            }
            self::$instance = new self(STDSCR);
        }
        return self::$instance;
    }

    protected function __construct($stdscr) {
        $this->ncurses_resource = $stdscr;
    }

}
