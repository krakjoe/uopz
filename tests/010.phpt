--TEST--
uopz_get_hook
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

var_dump(uopz_get_hook(Foo::class, "method"));
?>
--EXPECT--
bool(true)
object(Closure)#1 (1) {
  ["parameter"]=>
  array(1) {
    ["$arg"]=>
    string(10) "<required>"
  }
}
