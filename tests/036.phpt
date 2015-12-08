--TEST--
Test modifiers on functions
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {
	public function mine() {
		return $this->arg;
	}

	protected $arg;
}

uopz_function("Test", 'isStatic', function(){
	return __METHOD__;
}, ZEND_ACC_STATIC);

var_dump(Test::isStatic());
?>
--EXPECTF--
string(9) "{closure}"






