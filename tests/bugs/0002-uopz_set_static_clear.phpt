--TEST--
uopz_set_static with empty array
--SKIPIF--
<?php
include(__DIR__ . '/../skipif.inc');
?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public static function bar () {
		static $baz = null;

		return $baz;
	}
}

uopz_set_static ('Foo', 'bar', ["baz" => 1]);
var_dump(Foo::bar());
uopz_set_static ('Foo', 'bar', []);
var_dump(Foo::bar());
?>
--EXPECT--
int(1)
NULL
