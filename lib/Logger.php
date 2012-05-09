<?php

/**
 * Logger class
 */
class Logger {

    public static function debug() {
        $args = func_get_args();
        array_unshift($args, 'debug');
        call_user_func_array(array(__CLASS__, 'log'), $args);
    }

    public static function info() {
        $args = func_get_args();
        array_unshift($args, 'info');
        call_user_func_array(array(__CLASS__, 'log'), $args);
    }

    public static function error() {
        $args = func_get_args();
        array_unshift($args, 'error');
        call_user_func_array(array(__CLASS__, 'log'), $args);
    }

    public static function warning() {
        $args = func_get_args();
        array_unshift($args, 'warning');
        call_user_func_array(array(__CLASS__, 'log'), $args);
    }

    private static function log() {
        $args = func_get_args();
        $type = array_shift($args);
        $msg = array_shift($args);
        $namespace = array_shift($args);
        if ($namespace === null) {
            $namespace = 'none';
        }
        if (!empty($args)) {
            $msg .= ' ' . json_encode($args);
        }
        $msg = trim(preg_replace('|\s+|', ' ', $msg));
        error_log(date('Y/m/d:H:i:s') . " [$type] [$namespace] $msg\n", 3, Config::get('log_path', '/tmp/mace.log'));
    }

}
