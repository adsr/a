<?php

/**
 * Wrapper for ncurses functions
 */
class Ncurses {

    protected static $is_ncurses_initialized = false;

    private static $ncurses_window_methods = array(
        'delwin', 'getmaxyx', 'getyx', 'keypad', 'meta', 'mvwaddstr', 'new_panel',
        'waddch', 'waddstr', 'wattroff', 'wattron', 'wattrset', 'wborder', 'wclear',
        'wcolor_set', 'werase', 'wgetch', 'whline', 'wmouse_trafo', 'wmove',
        'wnoutrefresh', 'wrefresh', 'wstandend', 'wstandout', 'wvline'
    );

    private static $color_pairs = array();

    const COLOR_DEFAULT = 'default';

    private static $color_names = array(
        'black' => NCURSES_COLOR_BLACK,
        'white' => NCURSES_COLOR_WHITE,
        'red' => NCURSES_COLOR_RED,
        'green' => NCURSES_COLOR_GREEN,
        'yellow' => NCURSES_COLOR_YELLOW,
        'blue' => NCURSES_COLOR_BLUE,
        'cyan' => NCURSES_COLOR_CYAN,
        'magenta' => NCURSES_COLOR_MAGENTA
    );

    private static $default_color_fg;
    private static $default_color_bg;

    public static function makeWindow($rows, $cols, $line, $offset, Ncurses $parent_window = null) {
        self::ensureInitialized();
        if ($parent_window === null) {
            $parent_window = Ncurses_Stdscr::getInstance();
        }
        return new self($parent_window, $rows, $cols, $line, $offset);
    }

    public static function __callStatic($method_name, array $args) {
        self::ensureInitialized();
        $ncurses_fn = "ncurses_{$method_name}";
        return call_user_func_array($ncurses_fn, $args);
    }

    public static function getColorPair($fg, $bg) {
        //self::ensureInitialized();
        if (!isset(self::$color_names[$fg])) {
        }
        if (!isset(self::$color_names[$bg])) {
        }
        // TODO throw exceptions
        $fg_num = $fg === self::COLOR_DEFAULT ? self::$default_color_fg : self::$color_names[$fg];
        $bg_num = $bg === self::COLOR_DEFAULT ? self::$default_color_bg : self::$color_names[$bg];
        $pair_key = "{$fg_num}.{$bg_num}";
        if (!isset(self::$color_pairs[$pair_key])) {
            $pair_num = count(self::$color_pairs) + 1;
            ncurses_init_pair($pair_num, $fg_num, $bg_num);
            self::$color_pairs[$pair_key] = $pair_num;
        }
        Logger::debug("getColorPair: $fg/$bg is " . self::$color_pairs[$pair_key], __CLASS__);
        return self::$color_pairs[$pair_key];
    }

    private static function ensureInitialized() {
        if (self::$is_ncurses_initialized) {
            return;
        }
        self::$ncurses_window_methods = array_fill_keys(self::$ncurses_window_methods, 1);
        self::initNcurses();
        register_shutdown_function(function() {
            ncurses_end();
        });
        self::$is_ncurses_initialized = true;
    }

    protected static function initNcurses() {
        if (!defined('STDSCR')) {
            ncurses_init();
        }
        ncurses_start_color();
        ncurses_use_default_colors();
        ncurses_pair_content(0, self::$default_color_fg, self::$default_color_bg);
        ncurses_noecho();
        ncurses_cbreak();
        ncurses_raw();
        ncurses_curs_set(0);
        ncurses_savetty();
    }

    protected $parent;

    protected $ncurses_resource;

    public $rows;
    public $cols;
    public $line;
    public $offset;

    private $color_fg = 'default';
    private $color_bg = 'default';

    protected function __construct(Ncurses $parent_window, $rows, $cols, $line, $offset) {
        $this->parent = $parent_window;
        $max_rows = $this->parent->getNumRows();
        $max_cols = $this->parent->getNumCols();
        if (!is_int($rows) || !is_int($offset) || $rows < 1 || $rows + $offset > $max_rows) {
            //error
        }
        else if (!is_int($cols) || !is_int($line) || $cols < 1 || $cols + $line > $max_cols) {
            //error
        }
        $this->ncurses_resource = self::newwin($rows, $cols, $line, $offset);
        $this->rows = $rows;
        $this->cols = $cols;
        $this->line = $line;
        $this->offset = $offset;
    }

    public function __destruct() {
        $this->delwin();
    }

    public function __call($method_name, array $args) {
        self::ensureInitialized();
        $ncurses_fn = "ncurses_{$method_name}";
        if (isset(self::$ncurses_window_methods[$method_name])) {
            array_unshift($args, $this->ncurses_resource);
        }
        return call_user_func_array($ncurses_fn, $args);
    }

    public function getResource() {
        return $this->ncurses_resource;
    }

    public function getNumRows() {
        list($rows, $_) = $this->getDimensions();
        return $rows;
    }

    public function getNumCols() {
        list($_, $cols) = $this->getDimensions();
        return $cols;
    }

    public function getDimensions() {
        $rows = null;
        $cols = null;
        self::__call('getmaxyx', array(&$rows, &$cols));
        return array($rows, $cols);
    }

    public function setColor($fg, $bg) {
        if ($this->color_fg == $fg && $this->color_bg == $bg) {
            return;
        }
        $pair = self::getColorPair($fg, $bg);
        $this->wcolor_set($pair);
        $this->color_fg = $fg;
        $this->color_bg = $bg;
    }

    public function delwin() {
        if (is_resource($this->ncurses_resource)) {
            ncurses_delwin($this->ncurses_resource);
        }
    }

}
