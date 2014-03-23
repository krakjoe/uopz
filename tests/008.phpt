--TEST--
Test redefine
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
define("MY", 1);
uopz_redefine("MY", 100);
var_dump(MY);

class Test {
	const CON = 1;
}

uopz_redefine("Test", "CON", 10);
var_dump(Test::CON);
uopz_redefine("Test", "CON", 40);
var_dump(Test::CON);
uopz_undefine("Test", "CON");
uopz_redefine("Test", "CON", 60);
var_dump(Test::CON);
?>
--EXPECT--
int(100)
int(10)
int(40)
int(60)


