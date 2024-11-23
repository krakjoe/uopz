--TEST--
uopz_get_mock
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Bar {}

uopz_set_mock(Foo::class, Bar::class);

var_dump(uopz_get_mock(Foo::class));

uopz_set_mock(Foo::class, new Bar);

var_dump(uopz_get_mock(Foo::class));
?>
--EXPECTF--
string(3) "Bar"
object(Bar)#%d (0) {
}
