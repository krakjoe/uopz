--TEST--
get hook not case insensitive
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php

declare(strict_types=1);

function Stub() {
    return 123;
}

uopz_set_hook('Stub', function () {
    var_dump('hook called');
});

$a = uopz_get_hook('Stub');

var_dump(Stub());

var_dump($a);
?>
--EXPECT--
string(11) "hook called"
int(123)
object(Closure)#1 (0) {
}

