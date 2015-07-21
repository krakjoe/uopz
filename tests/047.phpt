--TEST--
Test uopz_flags operation
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {
	public function method() {
		return __CLASS__;
	}
}

$flags = uopz_flags("Test", "method", ZEND_ACC_FETCH);

var_dump((bool) (uopz_flags("Test", "method", ZEND_ACC_FETCH) & ZEND_ACC_PRIVATE));
var_dump((bool) (uopz_flags("Test", "method", ZEND_ACC_FETCH) & ZEND_ACC_STATIC));

var_dump(uopz_flags("Test", "method", $flags|ZEND_ACC_STATIC|ZEND_ACC_PRIVATE));

var_dump((bool) (uopz_flags("Test", "method", ZEND_ACC_FETCH) & ZEND_ACC_PRIVATE));
var_dump((bool) (uopz_flags("Test", "method", ZEND_ACC_FETCH) & ZEND_ACC_STATIC));

try {
	uopz_flags("Test", null, ZEND_ACC_STATIC);
} catch (Exception $ex) {
	printf("%s\n", $ex);
}

try {
	uopz_flags("Test", null, ZEND_ACC_PRIVATE);
} catch (Exception $ex) {
	printf("%s\n", $ex);
}
?>
--EXPECTF--
bool(false)
bool(false)
int(%d)
bool(true)
bool(true)
RuntimeException: failed to set or get flags of Test::, function does not exist in %s:19
Stack trace:
#0 %s(19): uopz_flags('Test', '', 1)
#1 {main}
RuntimeException: failed to set or get flags of Test::, function does not exist in %s:25
Stack trace:
#0 %s(25): uopz_flags('Test', '', 1024)
#1 {main}

