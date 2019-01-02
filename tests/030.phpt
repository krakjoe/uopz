--TEST--
init static method call self
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public static function method() {
		return self::internal();
	}

	private static function internal() {
		return false;
	}
}

class Bar {
	public static function method() {
		return self::internal();
	}

	private static function internal() {
		return true;
	}
}

var_dump(Foo::method());

uopz_set_mock(Foo::class, Bar::class);

var_dump(Foo::method());
?>
--EXPECTF--
bool(false)
bool(true)
