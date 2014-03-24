--TEST--
Test backup/restore
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_backup('fgets');
uopz_function('fgets', function(){
	return true;
});
var_dump(fgets());
uopz_restore('fgets');
fgets();
?>
--EXPECTF--
bool(true)

Warning: fgets() expects at least 1 parameter, 0 given in %s on line %d

