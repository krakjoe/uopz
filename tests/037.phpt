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
Fatal error: Call to protected method Test::existing() from context '' in %s on line %d


