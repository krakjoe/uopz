--TEST--
uopz_set_mock
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Bar {
	public static function thing() {
		return true;
	}
}

uopz_set_mock(Foo::class, Bar::class);

var_dump(new Foo);

var_dump(Foo::thing());

class Qux {
	public static function thing() {
		return true;
	}
}

uopz_set_mock(Foo::class, new Qux);

var_dump(new Foo);

var_dump(Foo::thing());
?>
--EXPECT--
object(Bar)#1 (0) {
}
bool(true)
object(Qux)#1 (0) {
}
bool(true)
