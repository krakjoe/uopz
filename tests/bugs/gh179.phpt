--TEST--
uopz_set_static() does not check that $static is an array
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
function foo() {
    static $a = "a";
}

uopz_set_static("foo", 42);
?>
--EXPECTF--
Fatal error: Uncaught InvalidArgumentException: unexpected parameter combination, expected (class, function, statics) or (function, statics) in %s:%d
%A
