--TEST--
uopz_extend
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {}
class Bar {}

uopz_extend(Foo::class, Bar::class);

$foo = new Foo;

var_dump($foo instanceof Bar);

try {
	uopz_extend(Foo::class, Bar::class);
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
bool(true)
string(29) "class Foo already extends Bar"
