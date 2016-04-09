--TEST--
Test uopz_get_mock
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Bar {}

uopz_set_mock(Foo::class, new Bar);

var_dump(uopz_get_mock(Foo::class));
?>
--EXPECT--
object(Bar)#1 (0) {
}
