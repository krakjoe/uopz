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
exception 'RuntimeException' with message 'attempt to set static on class entry, not allowed' in %s:%d
Stack trace:
#0 %s(%d): uopz_flags('Test', NULL, %d)
#1 {main}
exception 'RuntimeException' with message 'attempt to set public, private or protected on class entry, not allowed' in %s:%d
Stack trace:
#0 %s(%d): uopz_flags('Test', NULL, %d)
#1 {main}

