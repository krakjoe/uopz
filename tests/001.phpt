--TEST--
uopz_set_return
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function bar() {
		return false;	
	}
}

var_dump(uopz_set_return(Foo::class, "bar", true));

$foo = new Foo();

var_dump($foo->bar());

uopz_set_return(Foo::class, "bar", function() {
	return 2;
}, true);

var_dump($foo->bar());

try {
	uopz_set_return(Foo::class, "nope", 1);
} catch(Throwable $t) {
	var_dump($t->getMessage());
}

class Bar extends Foo {}

try {
	uopz_set_return(Bar::class, "bar", null);
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
bool(true)
bool(true)
int(2)
string(61) "failed to set return for Foo::nope, the method does not exist"
string(63) "failed to set return for Bar::bar, the method is defined in Foo"

