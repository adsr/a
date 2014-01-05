#!/usr/bin/php
<?php

// Define consts
define('MACE_VERSION', '0.1');
if ($_ENV['HOME']) {
    define('RC_LOCAL', rtrim($_ENV['HOME'], '/') . '/.macerc');
}
define('RC_DIST', __DIR__ . '/../etc/mace.rc');
define('RC_GLOBAL', '/etc/mace.rc');
define('LIB_DIR', __DIR__ . '/../lib/');
$version = MACE_VERSION;
$is_bootstrap = count(get_included_files()) > 1;

// Get command line args
$opt = $is_bootstrap ? array() : getopt('hc:');

// Show help if -h
if (isset($opt['h'])) {
    echo
<<<HELP
mace {$version}

Usage: mace [switches]

Switches:
    -c <path>  load config from path
    -h         show this help dialog


HELP;
    die();
}

// Get config path
if (isset($opt['c'])) {
    // From command line arg
    $rc_path = $opt['c'];
}
else if (defined('RC_LOCAL') && is_readable(RC_LOCAL)) {
    // From /home
    $rc_path = RC_LOCAL;
}
else if (is_readable(RC_GLOBAL)) {
    // From /etc
    $rc_path = RC_GLOBAL;
}
else if (is_readable(RC_DIST)) {
    // From distribution
    $rc_path = RC_DIST;
}
else {
    // Default config
    $rc_path = null;
}

// Require config
if ($rc_path !== null && !is_readable($rc_path)) {
    echo "Rc file not readable: $rc_path\n";
    exit(1);
}

// Include lib in PHP include path
set_include_path(LIB_DIR . PATH_SEPARATOR . get_include_path());

// Set up class autoloading
spl_autoload_register(function($class_name) {
    $path = explode('_', $class_name);
    require implode('/', $path) . '.php';
}, true);

// Set error handler
set_error_handler(function ($no, $str, $file, $line) {
    throw new ErrorException($str, $no, 0, $file, $line);
});

// Set uncaught exception handler
set_exception_handler(function($e) {
    if (class_exists('Ncurses_Stdscr')) {
        $stdscr = Ncurses_Stdscr::getInstance();
        $stdscr->clear();
        $stdscr->refresh();
        $stdscr->end();
    }
    echo (string)$e;
});

if (!$is_bootstrap) {
    // Instantiate
    if (!function_exists('ncurses_init')) {
        echo "Missing ncurses extension\n";
        exit(1);
    }
    $mace = new Mace();
    if ($rc_path) {
        $mace->setRcPath($rc_path);
    }
    $mace->run();
}
