--TEST--
fetch static W
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Bar {
	public static $property = true;
}

uopz_set_mock(Foo::class, Bar::class);

Foo::$property = false;

if (!Foo::$property) {
	echo "OK\n";
}
?>
--EXPECTF--
OK

