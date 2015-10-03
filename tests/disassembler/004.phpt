--TEST--
Test disassembler parameter types and return type
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(int $arg1, int $arg2) : int {
	return $arg1 + $arg2;
}, [1, 2]);
?>
--EXPECT--
int(3)
