--TEST--
Test modifiers on functions are copied by default
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {
	protected function existing() {
		return $this->arg;
	}

	protected $arg;
}

uopz_function("Test", 'existing', function(){
	return __METHOD__;
});

var_dump(Test::existing());
?>
--EXPECTF--
Fatal error: Uncaught Error: Call to protected method Test::existing() from context '' in %s:14
Stack trace:
#0 {main}
  thrown in %s on line 14


