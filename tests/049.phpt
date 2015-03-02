--TEST--
Test ancestry fix for uopz_restore issue #10 on github
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class A {
	public function method() {
		return true;
	}
}

class B extends A {}

/* this relies on 48.phpt passing */
uopz_function("A", "method", function(){
	return false;
}, ZEND_ACC_PUBLIC);

uopz_restore("A", "method");

$b = new B();
var_dump($b->method());
?>
--EXPECT--
bool(true)

