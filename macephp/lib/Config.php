<?php

/**
 * Config registry class
 */
class Config {

    /** @var array */
    private $config;

    /** @var Config singleton instance */
    private static $instance;

    /** @return Config get singleton */
    public static function getInstance() {
        if (!self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    /** Start with blank config registry */
    private function __construct() {
        $this->config = array();
    }

    /**
     * Gets a config key
     *
     * @param string $config_key config key in dot syntax, e.g., 'thing.property.subproperty'
     * @param mixed $default_value if config key not found, return this value
     * @return mixed
     */
    private function get($config_key, $default_value = null) {
        $keys = explode('.', $config_key);
        $config_ref = &$this->config;
        foreach ($keys as $key) {
            if (is_array($config_ref) && array_key_exists($key, $config_ref)) {
                $config_ref = &$config_ref[$key];
            }
            else {
                return $default_value;
            }
        }
        return $config_ref;
    }

    /**
     * Sets a config key
     *
     * @param string $config_key config key in dot syntax, e.g., 'thing.property.subproperty'
     * @param mixed $value
     */
    private function set($config_key, $value) {
        $keys = explode('.', $config_key);
        $config_ref = &$this->config;
        for ($i = 0, $l = count($keys); $i < $l; $i++) {
            $key = $keys[$i];
            if ($i === $l - 1) {
                $config_ref[$key] = $value;
            }
            else if (!array_key_exists($key, $config_ref)) {
                $config_ref[$key] = array();
            }
            $config_ref = &$config_ref[$key];
        }
    }

    /**
     * Magic for calling Config::get and Config::set
     */
    public static function __callStatic($method_name, array $args) {
        $instance = self::getInstance();
        return call_user_func_array(array($instance, $method_name), $args);
    }

}
