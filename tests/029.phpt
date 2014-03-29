--TEST--
Test compose trait
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
if (uopz_compose("myTrait", [], [
	"method" => function(){}
], ZEND_ACC_TRAIT)) {
	var_dump
		(trait_exists("myTrait"));	
}
?>
--EXPECT--
bool(true)

