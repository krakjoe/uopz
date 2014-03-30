--TEST--
Test uopz_delete cleans up magic
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {
	public function __toString() {
		return __CLASS__;
	}
}

uopz_delete("test", "__tostring");

$test = new Test();

var_dump((string) $test);
?>
--EXPECTF--
Catchable fatal error: Object of class Test could not be converted to string in %s on line %d

