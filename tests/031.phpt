--TEST--
init static method call ref method name
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public static function method() {
		return false;
	}
}

class Bar {
	public static function method() {
		return true;
	}
}

$string = sprintf("method");
$method = &$string;

var_dump(Foo::$method());

uopz_set_mock(Foo::class, Bar::class);

var_dump(Foo::$method());
?>
--EXPECTF--
bool(false)
bool(true)
