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

try {
	uopz_implement(Foo::class, Bar::class);
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECTF--
bool(true)
string(%d) "the class provided (%s) already has the interface %s"
string(%d) "the class provided (%s) is not an interface"
