--TEST--
init method call non string method
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function method() {
		return false;
	}
}

$foo = new Foo();
$method = 1;

var_dump($foo->$method());

?>
--EXPECTF--
Fatal error: Uncaught Error: Method name must be a string in %s:11
Stack trace:
#0 {main}
  thrown in %s on line 11
