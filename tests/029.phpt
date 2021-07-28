--TEST--
init method call object ref
--EXTENSIONS--
uopz
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
