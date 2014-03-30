--TEST--
Test uopz_function with global function and modifiers
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_function('test', function($arg){
	return true;
}, ZEND_ACC_STATIC);

var_dump(test(1));
?>
--EXPECTF--
bool(true)
