--TEST--
Test sane composition of normal classes
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Base {}
class Other {}

uopz_compose(Concrete::class, [Base::class, Other::class]);
?>
--EXPECTF--
Fatal error: Uncaught exception '%s' with message 'class Concrete may not extend Other, parent of Concrete already set to Base' in %s:%d
Stack trace:
#0 %s(%d): uopz_compose('Concrete', Array)
#1 {main}
  thrown in %s on line %d

