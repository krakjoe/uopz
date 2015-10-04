--TEST--
Test disassembler use
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

$three = 3;

run_disassembler_test(function(int $one, int $two) use($three) : int {
	return $one + $two + $three;
}, [1, 2, 3]);
?>
--EXPECT--
int(6)

