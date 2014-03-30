--TEST--
Test creation of properties when composing
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
/*
class A {
	public $name;
}
*/
uopz_compose("A", [/* no inheritance */], [/* no methods */], [
	"name" => ZEND_ACC_PUBLIC
], ZEND_ACC_CLASS);

$a = new A(new stdClass());

var_dump($a, $a->name);
?>
--EXPECTF--
object(A)#%d (1) {
  ["name"]=>
  NULL
}
NULL


