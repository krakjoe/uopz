--TEST--
init static method call constructor with constructor
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
class Foo {}

class Mock {
	public function __construct() {
		echo "OK\n";
	}
}

class Test extends Foo {

	public function __construct() {

		uopz_set_mock(Foo::class, Mock::class);

		parent::__construct();
	}
}

var_dump(new Test());
--EXPECTF--
OK
object(Test)#%d (0) {
}

