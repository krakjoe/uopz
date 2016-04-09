--TEST--
uopz_set_hook
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function method($arg) {
		
	}
}

var_dump(uopz_set_hook(Foo::class, "method", function($arg){
	var_dump($arg);
	var_dump($this);
}));

$foo = new Foo();

$foo->method(true);
?>
--EXPECT--
bool(true)
bool(true)
object(Foo)#2 (0) {
}
