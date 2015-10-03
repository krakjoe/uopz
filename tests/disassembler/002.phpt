--TEST--
Test disassembler basics
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function($arg1, $arg2){
	return $arg1 + $arg2;
}, [1, 2]);
?>
--EXPECT--
int(3)
