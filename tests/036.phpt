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

uopz_function("Test", 'isProtected', function(){
	return __METHOD__;
}, ZEND_ACC_PROTECTED);

var_dump(Test::isProtected());
?>
--EXPECTF--
Fatal error: Call to protected method Test::isProtected() from context '' in %s on line %d




