--TEST--
Test disassembler foreach
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('disassembler.inc');

run_disassembler_test(function(array $things) {
	while ($thing = array_shift($things))
		var_dump($thing);
	return [];
}, [[1,2,3]]);
?>
--EXPECT--
int(1)
int(2)
int(3)
array(0) {
}

