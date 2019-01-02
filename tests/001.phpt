--TEST--
uopz_set_return
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public function bar(int $arg) : int {
		return $arg;
	}

	public static function qux(int $arg) : int {
		return $arg;
	}
}

var_dump(uopz_set_return(Foo::class, "bar", true));

$foo = new Foo();

var_dump($foo->bar(1));

uopz_set_return(Foo::class, "bar", function(int $arg) : int {
	return $arg * 2;
}, true);

var_dump($foo->bar(2));

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

var_dump($foo::qux(10));

uopz_set_Return(Foo::class, "qux", function(int $arg) : int {
	return $arg * 2;
}, true);

var_dump($foo::qux(20));
?>
--EXPECT--
bool(true)
bool(true)
int(4)
string(61) "failed to set return for Foo::nope, the method does not exist"
string(63) "failed to set return for Bar::bar, the method is defined in Foo"
int(10)
int(40)
