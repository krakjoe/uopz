--TEST--
static unset
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {}

class Bar {
	public static $property = true;

	public static function call() {
		unset(self::$property);
	}
}

uopz_set_mock(Foo::class, Bar::class);

try {
	unset(Foo::$property);
} catch (Error $e) {
	var_dump($e->getMessage());
}

try {
	Foo::call();
} catch (Error $e) {
	var_dump($e->getMessage());
}

uopz_unset_mock(Foo::class);

try {
	unset(Foo::$property);
} catch (Error $e) {
	var_dump($e->getMessage());
}

try {
	unset(Qux::$property);
} catch (Error $e) {
	var_dump($e->getMessage());
}
?>
--EXPECTF--
string(%d) "Attempt to unset static property Bar::%s"
string(%d) "Attempt to unset static property Bar::%s"
string(%d) "Attempt to unset static property Foo::%s"
string(%d) "Class '%s' not found"

