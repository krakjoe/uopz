--TEST--
Test protected check
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Test {
	protected function one() {
		return '321';
	}

	public function __call($name, $args) {
		return $this->one();
	}
}

uopz_function(Test::class, 'one', function() { return 'redefined'; });

var_dump((new Test())->CallMagic());
?>
--EXPECT--
string(9) "redefined"
