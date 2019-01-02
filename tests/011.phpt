--TEST--
uopz_unset_hook
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public function method() {
		return false;
	}
}

var_dump(uopz_set_hook(Foo::class, "method", function(){
	var_dump(true);
}));

$foo = new Foo();

var_dump($foo->method());

uopz_unset_hook(Foo::class, "method");

var_dump($foo->method());
?>
--EXPECT--
bool(true)
bool(true)
bool(false)
bool(false)
