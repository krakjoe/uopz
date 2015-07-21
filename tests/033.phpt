--TEST--
Test sane use of traits and interfaces
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
trait Base {}
interface Other {}

uopz_compose("Concrete",
	["Base", "Other"], [], [], ZEND_ACC_TRAIT);

var_dump(new Concrete());
?>
--EXPECTF--
Fatal error: Uncaught %s: trait Concrete can not implement interface Other, not allowed in %s:%d
Stack trace:
#0 %s(%d): uopz_compose('Concrete', Array, Array, Array, %d)
#1 {main}
  thrown in %s on line %d


