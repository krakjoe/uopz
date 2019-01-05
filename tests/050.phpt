--TEST--
cuf with mock and object
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

$foo = new Foo();

uopz_set_mock(Foo::class, new class {
	public function method() {
		return true;
	}
});

if (call_user_func([$foo, "method"])) {
	echo "OK";
}
?>
--EXPECTF--
OK

