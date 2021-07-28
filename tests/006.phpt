--TEST--
uopz_unset_mock
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Bar {}

uopz_set_mock(Foo::class, Bar::class);

var_dump(uopz_get_mock(Foo::class));

uopz_unset_mock(Foo::class);

var_dump(uopz_get_mock(Foo::class));

try {
	uopz_unset_mock(Foo::class);
} catch (RuntimeException $ex) {
	echo "OK\n";
}
?>
--EXPECT--
string(3) "Bar"
NULL
OK
