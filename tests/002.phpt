--TEST--
Test uopz_set_mock (mock existing)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Bar {}

uopz_set_mock(Foo::class, Bar::class);

var_dump(new Foo());

uopz_unset_mock(Foo::class);

var_dump(new Foo());
?>
--EXPECTF--
object(Bar)#%d (%d) {
}

Fatal error: Uncaught Error: Class 'Foo' not found in %s:10
Stack trace:
#0 {main}
  thrown in %s on line 10
