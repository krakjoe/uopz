--TEST--
fetch class string no mock
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
class Foo {
	public static function qux() {
		return true;
	}
}

$class = sprintf("Foo");

var_dump($class::qux());
--EXPECT--
bool(true)

