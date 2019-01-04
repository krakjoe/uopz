--TEST--
property fetch W
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

$val = 42;

$foo->property =& $val;

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

