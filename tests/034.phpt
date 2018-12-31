--TEST--
fetch class constant IS_UNUSED op1 with mock
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
class Test {
	public static function run() {
		uopz_set_mock(self::class, Mock::class);

		var_dump(self::VALUE);
	}
}

class Mock extends Test {
	const VALUE = true;
}

Test::run();
--EXPECTF--
bool(true)
