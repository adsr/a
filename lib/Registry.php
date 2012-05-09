<?php

/**
 * Registry class
 */
class Registry {

    const SCOPE_GLOBAL = 'global';
    const SCOPE_CONTEXT = 'context';
    const SCOPE_CONTROL = 'control';
    const SCOPE_COMMAND = 'command';

    const VAR_PREFIX = '$';

    const SCOPE_GLOBAL_KEY = '0';

    /** @var ContextStack */
    private $context_stack;

    /** @var array */
    private $data;

    /** Ctor */
    public function __construct(ContextStack $context_stack) {
        $this->context_stack = $context_stack;
        $this->data = array(
            self::SCOPE_GLOBAL => array(),
            self::SCOPE_CONTEXT => array(),
            self::SCOPE_CONTROL => array(),
            self::SCOPE_COMMAND => array(),
        );
    }

    /** @return bool */
    public function isVarPrefix($char) {
        return $char == self::VAR_PREFIX ? $char : false;
    }

    /** Sets a var in the registry */
    public function setVar($name, $value, $scope_var = null) {
        list($scope_type, $scope_key) = $this->resolveScope($scope_var);
        $this->data[$scope_type][$scope_key][$name] = $value;
        Logger::debug("setVar [$scope_type][$scope_key][$name] = $value", __CLASS__);
    }

    /** Unsets a var in the registry */
    public function clearVar($name, $scope_var) {
        list($scope_type, $scope_key) = $this->resolveScope($scope_var);
        if (isset($this->data[$scope_type][$scope_key][$name])) {
            unset($this->data[$scope_type][$scope_key][$name]);
        }
    }

    /** Unsets an entire scope */
    public function clearScope($scope_var) {
        list($scope_type, $scope_key) = $this->resolveScope($scope_var);
        if (isset($this->data[$scope_type][$scope_key])) {
            unset($this->data[$scope_type][$scope_key]);
        }
    }

    /** Resolves scope */
    private function resolveScope($scope_var) {
        $scope_type = null;
        $scope_key = null;
        if ($scope_var === self::SCOPE_GLOBAL) {
            $scope_type = self::SCOPE_GLOBAL;
            $scope_key = self::SCOPE_GLOBAL_KEY;
        }
        else if ($scope_var === self::SCOPE_CONTEXT) {
            $scope_type = self::SCOPE_CONTEXT;
            $scope_key = $this->context_stack->getContextName();
        }
        else if ($scope_var instanceof Command) {
            $scope_type = self::SCOPE_COMMAND;
            $scope_key = get_class($scope_var);
        }
        else if ($scope_var === null || $scope_var === self::SCOPE_CONTROL) {
            $scope_type = self::SCOPE_CONTROL;
            $scope_key = get_class($this->context_stack->getActiveControl());
        }
        else if ($scope_var instanceof Control) {
            $scope_type = self::SCOPE_CONTROL;
            $scope_key = get_class($scope_var);
        }
        else {
            throw new InvalidArgumentException('Invalid scope');
        }
        return array($scope_type, $scope_key);
    }

    /** Evalutes a variable */
    public function evaluate($var_str, $scope_var = null) {
        $var_prefix = $this->isVarPrefix(substr($var_str, 0, 1));
        if (!$var_prefix) {
            Logger::warning("Could not evaluate variable {$var_str}; assuming string", __CLASS__);
            return $var_str;
        }
        list($scope_type, $scope_key) = $this->resolveScope($scope_var);
        Logger::debug("resolveScope: $scope_type, $scope_key", __CLASS__);
        $var_name = substr($var_str, 1);
        if (isset($this->data[$scope_type][$scope_key][$var_name])) {
            return (string)$this->data[$scope_type][$scope_key][$var_name];
        }
        else if (isset($this->data[self::SCOPE_GLOBAL][self::SCOPE_GLOBAL_KEY][$var_name])) {
            return (string)$this->data[self::SCOPE_GLOBAL][self::SCOPE_GLOBAL_KEY][$var_name];
        }
        Logger::warning("Variable {$var_str} not found in [$scope_type][$scope_key]", __CLASS__);
        return '';
    }

}
