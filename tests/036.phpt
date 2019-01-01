--TEST--
init static method call constructor with no constructor
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
class Foo {}
class Bar {}

class Test extends Foo {

	public function __construct() {
		uopz_set_mock(Foo::class, Bar::class);

		parent::__construct();
	}
}

var_dump(new Test());
--EXPECTF--
Fatal error: Uncaught Error: Cannot call constructor in %s:10
Stack trace:
#0 %s(14): Test->__construct()
#1 {main}
  thrown in %s on line 10
