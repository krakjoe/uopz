--TEST--
uopz_set_mock
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Bar {}

uopz_set_mock(Foo::class, Bar::class);

var_dump(new Foo);

class Qux {}

uopz_set_mock(Foo::class, new Qux);

var_dump(new Foo);
?>
--EXPECT--
object(Bar)#1 (0) {
}
object(Qux)#1 (0) {
}
