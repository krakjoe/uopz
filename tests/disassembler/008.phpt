--TEST--
Test disassembler brk
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(int $arg) : int {
	while ($arg < 20)
		$arg++;
	return $arg;
}, [2]);
?>
--EXPECT--
int(20)

