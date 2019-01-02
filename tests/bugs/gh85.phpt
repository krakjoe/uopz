--TEST--
cuf/cufa wierdness
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php

function myfunc($param)
{
    print $param . " world!\n";
}

$closure = function($param) {
    print $param . " universe!\n";
};

// replace myfunc
uopz_set_return("myfunc", $closure, true);

myfunc("hello");
call_user_func("myfunc", "hello");
call_user_func_array("myfunc", ["hello"]);
--EXPECT--
hello universe!
hello universe!
hello universe!

