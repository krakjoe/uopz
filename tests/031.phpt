--TEST--
Test sane composition of interfaces
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
interface Base {}
interface Other {}

uopz_compose("Concrete", ["Base", "Other"], ZEND_ACC_INTERFACE);
?>
--EXPECTF--
Fatal error: Uncaught exception '%s' with message 'interface Concrete may not extend Other, parent of Concrete already set to Base' in %s:%d
Stack trace:
#0 %s(%d): uopz_compose('Concrete', Array, %d)
#1 {main}
  thrown in %s on line %d

