--TEST--
fetch class string ref no mock
--EXTENSIONS--
uopz
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

