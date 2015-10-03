--TEST--
Test disassembler basics
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function($arg){
	return $arg;
}, [true]);
?>
--EXPECT--
bool(true)
