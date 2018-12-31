--TEST--
new IS_UNUSED op1 with mock
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
class Test {
	public static function run() {
		uopz_set_mock(self::class, Mock::class);

		$mock = new self();

		var_dump($mock->method());
	}
}

class Mock extends Test {
	public static function method() {
		return true;
	}
}

Test::run();
--EXPECTF--
bool(true)
