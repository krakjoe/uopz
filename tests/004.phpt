--TEST--
Test uopz_redefine cache reset
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Foo {
	const BAR = 1;

	public function __construct() {
		var_dump(self::BAR);
		uopz_redefine(self::class, "BAR", 0);
		var_dump(self::BAR);
		uopz_redefine(self::class, "BAR", 1);
		var_dump(self::BAR);
	}
}

new Foo;
?>
--EXPECT--
int(1)
int(0)
int(1)
