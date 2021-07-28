--TEST--
fetch class string no mock
--EXTENSIONS--
uopz
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

