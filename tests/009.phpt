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

class Bar extends Foo {}

try {
	uopz_set_hook(Bar::class, "method", function(){});
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
bool(true)
bool(true)
object(Foo)#2 (0) {
}
string(64) "failed to set hook for Bar::method, the method is defined in Foo"

