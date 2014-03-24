--TEST--
Test auto restore user functions
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function original() { return false; }
uopz_backup('original');
uopz_function('original', function(){
	return true;
});
var_dump(original());
?>
--EXPECTF--
bool(true)

