--TEST--
uopz_extend
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {}
class Bar {}

interface Qux {}

trait FooTrait {}
trait BarTrait {}

final class FinalClass {}

uopz_extend(Foo::class, Bar::class);

$foo = new Foo;

var_dump($foo instanceof Bar);

try {
	uopz_extend(Foo::class, Bar::class);
} catch (RuntimeException $t) {
	var_dump($t->getMessage());
}

try {
	uopz_extend(Qux::class, Foo::class);
} catch (RuntimeException $t) {
	var_dump($t->getMessage());
}

try {
	uopz_extend(FooTrait::class, Foo::class);
} catch (RuntimeException $t) {
	var_dump($t->getMessage());
}

var_dump(uopz_extend(FooTrait::class, BarTrait::class));

var_dump(uopz_extend(FinalClass::class, Foo::class));
?>
--EXPECTF--
bool(true)
string(%d) "the class provided (%s) already extends %s"
string(%d) "the interface provided (%s) cannot extend %s, because %s is not an interface"
string(%d) "the trait provided (%s) cannot extend %s, because %s is not a trait"
bool(true)
bool(true)
