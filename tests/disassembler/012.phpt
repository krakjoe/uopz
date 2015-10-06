--TEST--
Test disassembler while
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(array $things) {
	while (($thing = array_shift($things))) {
		var_dump($thing);
	}
}, [[1,2,3]]);
?>
--EXPECT--
int(1)
int(2)
int(3)
NULL

