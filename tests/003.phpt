--TEST--
Test uopz_set_mock (neither existing)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_set_mock(Foo::class, Bar::class);

var_dump(new Foo());
?>
--EXPECTF--
Fatal error: Uncaught Error: Class 'Foo' not found in %s:4
Stack trace:
#0 {main}
  thrown in %s on line 4
