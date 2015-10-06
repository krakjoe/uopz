--TEST--
Test disassembler for
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(array $things) {
	for ($i = 0; $i < count($things); $i++)
		var_dump($things[$i]);
}, [[1,2,3]]);
?>
--EXPECT--
int(1)
int(2)
int(3)
NULL

