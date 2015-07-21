--TEST--
Test compose error conditions (interface with properties)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_compose("A", [], [], ["property" => ZEND_ACC_PUBLIC], ZEND_ACC_INTERFACE);
?>
--EXPECTF--
Fatal error: Uncaught %s: interfaces can not have properties in %s:%d
Stack trace:
#0 %s(%d): uopz_compose('A', Array, Array, Array, %d)
#1 {main}
  thrown in %s on line %d


