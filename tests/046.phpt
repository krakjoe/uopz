--TEST--
Test uopz_delete cleans up magic
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {}

uopz_function(Test::class, "__toString", function(){
	return Test::class;
});

$test = new Test();
var_dump((string) $test);
uopz_delete("test", "__tostring");
var_dump((string) $test);
?>
--EXPECTF--
string(4) "Test"

Catchable fatal error: Object of class Test could not be converted to string in %s on line %d

