--TEST--
Test uopz_get_hook
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class Foo {
	public function method() {}
}

uopz_set_hook(Foo::class, "method", function(){});

var_dump(uopz_get_hook(Foo::class, "method"));
?>
--EXPECT--
object(Closure)#1 (0) {
}

