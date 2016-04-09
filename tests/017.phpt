--TEST--
Test uopz_set_mock with objects
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Foo {}
class Bar {}

uopz_set_mock(Foo::class, new Bar);

var_dump(new Foo);

var_dump(new Foo);
?>
--EXPECT--
object(Bar)#1 (0) {
}
object(Bar)#1 (0) {
}
