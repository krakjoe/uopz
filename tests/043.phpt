--TEST--
Test compose error conditions (interface with properties)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_compose("A", [], [], ["property" => ZEND_ACC_PUBLIC], ZEND_ACC_INTERFACE);
?>
--EXPECTF--
Fatal error: Uncaught exception '%s' with message 'interfaces can not have properties' in %s:%d
Stack trace:
#0 %s(%d): uopz_compose('A', Array, Array, Array, 128)
#1 {main}
  thrown in %s on line %d


