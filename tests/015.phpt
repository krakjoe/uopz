--TEST--
uopz_implement
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
interface Foo {}
class Bar {}

uopz_implement(Bar::class, Foo::class);

$bar = new Bar;

var_dump($bar instanceof Foo);

try {
	uopz_implement(Bar::class, Foo::class);
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
bool(true)
string(54) "the class provided (Bar) already has the interface Foo"
