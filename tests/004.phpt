--TEST--
uopz_set_mock
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Bar {
	const THING = 5;

	public static function thing() {
		return 10;
	}
}

uopz_set_mock(Foo::class, Bar::class);

var_dump($foo = new Foo);

var_dump(FOO::THING);

var_dump($foo::THING);

var_dump(Foo::thing());

class Qux {
	const THING = 50;

	public static function thing() {
		return 20;
	}
}

uopz_set_mock(Foo::class, new Qux);

var_dump($foo = new Foo);

var_dump(FOO::THING);

var_dump($foo::THING);

var_dump(Foo::thing());
?>
--EXPECT--
object(Bar)#1 (0) {
}
int(5)
int(5)
int(10)
object(Qux)#2 (0) {
}
int(50)
int(50)
int(20)
