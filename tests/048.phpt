--TEST--
Test ancestry fix for issue #10 on github
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

uopz_function("A", "method", function(){
	return false;
}, ZEND_ACC_PUBLIC);

$b = new B();
var_dump($b->method());
?>
--EXPECT--
bool(false)

