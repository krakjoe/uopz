--TEST--
Test bug in finding private user functions
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {
	private static function method() {
		return 5;
	}
}

$original = uopz_copy("Test", "method");
uopz_function("Test", "method", function() use($original) {
	return 5 * $original();
}, ZEND_ACC_STATIC);
var_dump(Test::method());
?>
--EXPECT--
int(25)

