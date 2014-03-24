--TEST--
Test auto restore internals
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_backup('fgets');
uopz_function('fgets', function(){
	return true;
});
var_dump(fgets());
?>
--EXPECTF--
bool(true)

