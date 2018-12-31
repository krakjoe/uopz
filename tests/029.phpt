--TEST--
init method call object ref
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function method() {
		return false;
	}
}

$object = new Foo();
$foo = &$object;

var_dump($foo->method());
?>
--EXPECTF--
bool(false)
