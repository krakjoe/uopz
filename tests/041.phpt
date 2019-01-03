--TEST--
fetch static R
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

if (Foo::$property) {
	echo "OK\n";
}
?>
--EXPECTF--
OK

