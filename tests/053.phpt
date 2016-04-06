--TEST--
Test restoration of magic methods
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

class Test {
	public $s = 'def';

	public function __construct($x) {
		$this->s = $x;
	}
}

uopz_function(Test::class, '__construct', function() { $this->s = 'redefined'; });
var_dump((new Test('vvv')));

uopz_restore(Test::class, '__construct');
var_dump((new Test('vvv')));
?>
--EXPECT--
object(Test)#1 (1) {
  ["s"]=>
  string(9) "redefined"
}
object(Test)#1 (1) {
  ["s"]=>
  string(3) "vvv"
}
