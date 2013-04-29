<?php

$lines = explode("\n", file_get_contents('atto.c'));
ob_start();

$all_funcs = array();

foreach ($lines as $line) {
    $matches = array();
    if (preg_match('/^([a-z][a-z_*]+)\s+([a-z_]+)\(([^)]+)\);$/i', $line, $matches)) {
        list(, $func_rtype, $func_name, $func_args) = $matches;
        $func_args = array_map(function($pair) {
            return preg_split('/\s+/', $pair);
        }, array_map('trim', explode(',', $func_args)));
        if (preg_match('/^(_|lapi_)/', $func_name)) {
            continue;
        }
        $all_funcs[] = array(null, $func_rtype, $func_name, $func_args);
    }
}

echo "int lapi_init(lua_State* L);\n";
foreach ($all_funcs as $func) {
    list(, $func_rtype, $func_name, $func_args) = $func;
    echo "int lapi_{$func_name}(lua_State* L);\n";
}
echo "\n\n";

echo "int lapi_init(lua_State* L) {\n";
echo "    L = luaL_newstate();\n";
echo "    luaL_openlibs(L);\n";
echo "    lua_settop(L, 0);\n";
foreach ($all_funcs as $func) {
    list(, $func_rtype, $func_name, $func_args) = $func;
    echo "    lua_pushcfunction(L, lapi_{$func_name});\n";
    echo "    lua_setglobal(L, \"{$func_name}\");\n";
}
echo "    return ATTO_RC_OK;";
echo "}\n\n";

foreach ($all_funcs as $func) {
    list(, $func_rtype, $func_name, $func_args) = $func;
    echo "int lapi_{$func_name}(lua_State* L) {\n";
    foreach ($func_args as &$func_arg) {
        $type = $func_arg[0];
        $identifier = $func_arg[1];
        $is_ret = preg_match('/^ret_/', $identifier);
        $func_arg[2] = $is_ret;
        if ($is_ret && substr($type, -1) == '*') {
            $type = substr($type, 0, -1);
        }
        echo "    {$type} {$identifier};\n";
    }
    echo "    {$func_rtype} retval;\n";
    $arg_i = 1;
    $call_args = array();
    $ret_args = array();
    foreach ($func_args as &$func_arg) {
        $type = $func_arg[0];
        $identifier = $func_arg[1];
        $check_type = null;
        if ($func_arg[2]) {
            $call_args[] = '&' . $identifier;
            $ret_args[] = $func_arg;
            continue;
        }
        $call_args[] = $identifier;
        $check_type = c_to_lua_type($type, $identifier);
        if ($check_type == 'lightuserdata') {
            echo "    luaL_checktype(L, {$arg_i}, LUA_TLIGHTUSERDATA);\n";
            echo "    {$func_arg[1]} = ($type)lua_touserdata(L, {$arg_i});\n";
            echo "    if (!{$func_arg[1]}) {\n";
            echo "        lua_pushinteger(L, 0);\n";
            echo "        return 1;\n";
            echo "    }\n";
        } else if ($check_type == 'function') {
            echo "    luaL_checktype(L, {$arg_i}, LUA_TFUNCTION);\n";
            echo "    lua_pushvalue(L, {$arg_i});\n";
            echo "    if (({$func_arg[1]} = luaL_ref(L, LUA_REGISTRYINDEX)) == LUA_REFNIL) {\n";
            echo "        lua_pushinteger(L, 0);\n";
            echo "        return 1;\n";
            echo "    }\n";
        } else {
            echo "    {$func_arg[1]} = " . ($check_type == 'string' ? '(char*)' : '') . "luaL_check{$check_type}(L, {$arg_i});\n";
        }
        $arg_i += 1;
    }
    echo "    retval = {$func_name}(" . implode(', ', $call_args) . ");\n";
    $push_type = c_to_lua_type($func_rtype);
    echo "    lua_push{$push_type}(L, retval);\n";
    foreach ($ret_args as $ret_arg) {
        $push_type = c_to_lua_type($ret_arg[0]);
        echo "    lua_push{$push_type}(L, {$ret_arg[1]});\n";
    }
    echo "    return " . (1 + count($ret_args)) . ";\n";
    echo "}\n\n";
}

$lapi_c = ob_get_clean();

file_put_contents('lapi.c', $lapi_c);

function c_to_lua_type($type, $identifier = '') {
    $type = str_replace('*', '', $type);
    if ($type == 'char') {
        return 'string';
    } else if ($type == 'int' && preg_match('/^fn_/', $identifier)) {
        return 'function';
    } else if ($type == 'int') {
        return 'integer';
    } else if ($type == 'float' || $type == 'double') {
        return 'number';
    } else {
        return 'lightuserdata';
    }
}
