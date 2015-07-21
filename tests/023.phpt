--TEST--
Test backup/restore internal methods
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
var_dump(method_exists("Exception", "__toString"));

var_dump(uopz_backup('Exception', '__toString'));
uopz_function('Exception', '__toString', function(){
	return "ok";
});

$ex = new Exception();
echo (string) $ex;

var_dump(uopz_restore('Exception', '__toString'));
?>
--EXPECT--
ok

