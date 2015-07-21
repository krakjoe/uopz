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
Fatal error: Uncaught Error: Call to protected method Test::isProtected() from context '' in %s:14
Stack trace:
#0 {main}
  thrown in %s on line 14





