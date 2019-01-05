--TEST--
cuf with mock and class
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public function method() {
		return false;
	}
}

class Bar {
	public function method() {
		return true;
	}
}

$foo = new Foo();

uopz_set_mock(Foo::class, Bar::class);

if (call_user_func([$foo, "method"])) {
	echo "OK";
}
?>
--EXPECTF--
OK

