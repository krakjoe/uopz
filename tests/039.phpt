--TEST--
fetch class string ref no mock
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
class Foo {
	public static function qux() {
		return true;
	}
}

$string = sprintf("Foo");
$class = &$string;

var_dump($class::qux());
--EXPECT--
bool(true)

