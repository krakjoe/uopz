--TEST--
Test sensible extend operation
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class A extends B {}
class B {}

uopz_extend("A", "B");
?>
--EXPECTF--
Fatal error: Uncaught exception '%s' with message 'class A already extends B' in %s:%d
Stack trace:
#0 %s(%d): uopz_extend('A', 'B')
#1 {main}
  thrown in %s on line %d


