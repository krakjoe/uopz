--TEST--
Test uopz_set_mock (both existing)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Foo {}
class Bar {}

uopz_set_mock(Foo::class, Bar::class);

var_dump(new Foo());

uopz_unset_mock(Foo::class);

var_dump(new Foo());
?>
--EXPECTF--
object(Bar)#%d (%d) {
}
object(Foo)#%d (%d) {
}



