--TEST--
Test disassembler method
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

class Foo {
	private $qux;

	public function __construct() {
		$this->qux = true;
	}
	
	public function bar() {
		return $this->qux;
	}
}

run_disassembler_method_test([new Foo(), "bar"], []);
?>
--EXPECT--
bool(true)

