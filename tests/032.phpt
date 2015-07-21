--TEST--
Test sane composition of traits
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
trait Base {}
trait Other {}

uopz_compose("Concrete",
	["Base", "Other"], [], [], ZEND_ACC_TRAIT);

var_dump(class_uses("Concrete"));

new Concrete();
?>
--EXPECTF--
array(2) {
  ["Base"]=>
  string(4) "Base"
  ["Other"]=>
  string(5) "Other"
}

Fatal error: Uncaught Error: Cannot instantiate trait Concrete in %s:10
Stack trace:
#0 {main}
  thrown in %s on line 10


