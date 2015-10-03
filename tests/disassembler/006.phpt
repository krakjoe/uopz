--TEST--
Test disassembler coercion with parameter and return types
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(string $arg1, string $arg2) : array {
	return [$arg1, $arg2];
}, [1, 2]);
?>
--EXPECT--
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}

