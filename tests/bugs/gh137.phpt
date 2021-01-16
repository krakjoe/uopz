--TEST--
no segmentation fault on dynamic function call
--SKIPIF--
<?php
include(__DIR__ . '/../skipif.inc');
?>
--FILE--
<?php

declare(strict_types=1);

class Test {
    public static function __callStatic($name, $args)
    {
        return 'static_call ' . $name . ' with ' . implode('|', $args);
    }

    public function __call($name, $args)
    {
        return 'call ' . $name . ' with ' . implode('|', $args);
    }
}

var_dump(call_user_func_array([new Test, 'test'], [1, 2, 3]));
var_dump(call_user_func_array([Test::class, 'test'], [1, 2, 3]));
var_dump(call_user_func([new Test, 'test'], 1, 2, 3));
var_dump(call_user_func([Test::class, 'test'], 1, 2, 3));
?>
--EXPECT--
string(20) "call test with 1|2|3"
string(27) "static_call test with 1|2|3"
string(20) "call test with 1|2|3"
string(27) "static_call test with 1|2|3"
