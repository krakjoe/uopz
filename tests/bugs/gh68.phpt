--TEST--
github #68: uopz_set_mock should not hang when mock with no-arg constructor is called with args
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class X {
	public function foo() {
		echo "X\n";
	}
}

class Y {
	public function foo() {
		echo "Y\n";
	}
}

uopz_set_mock(X::class, new class extends X {
	public function __construct() {}

	public function foo() {
		echo "anonymous\n";
	}
});

$x = new X(0,1,"test");
$x->foo();

uopz_unset_mock(X::class);
uopz_set_mock(X::class, new Y);

$x = new X(0);
$x->foo();

?>
--EXPECT--
anonymous
Y
