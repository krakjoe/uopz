--TEST--
property fetch IS
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {}

$foo = new Foo();

uopz_set_mock(Foo::class, new class extends Foo {
	public $property;
});

$foo->property = 42;

if (isset($foo->property)) {
	echo "OK1\n";
}

uopz_unset_mock(Foo::class);

if (!isset($foo->property)) {
	echo "OK2";
}
?>
--EXPECTF--
OK1
OK2

