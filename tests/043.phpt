--TEST--
fetch static RW
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Bar {
	public static $property = 0;
}

uopz_set_mock(Foo::class, Bar::class);

Foo::$property += 42;

if (Foo::$property == 42) {
	echo "OK\n";
}
?>
--EXPECTF--
OK

