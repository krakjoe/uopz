--TEST--
Test uopz_set_hook/uopz_unset_hook
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Foo {
	public function __construct() {
		$this->method();
	}

	private function method() {
		echo "After Hook\n";
	}
}

uopz_set_hook(Foo::class, "method", function(){
	echo "In Hook\n";
});

new Foo;

uopz_unset_hook(Foo::class, "method");

new Foo;
?>
--EXPECT--
In Hook
After Hook
After Hook


