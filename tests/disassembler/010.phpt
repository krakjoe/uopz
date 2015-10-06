--TEST--
Test disassembler foreach
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(array $things) {
	foreach ($things as $thing)
		var_dump($thing);
}, [[1,2,3]]);
?>
--EXPECT--
int(1)
int(2)
int(3)
NULL

