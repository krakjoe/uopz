--TEST--
property fetch R
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {}

$foo = new Foo();

uopz_set_mock(Foo::class, new class extends Foo {
	public $property = true;
});

if ($foo->property) {
	echo "OK\n";
}

uopz_unset_mock(Foo::class);

if (!isset($foo->property)) {
	echo "OK";
}
?>
--EXPECTF--
OK
OK

