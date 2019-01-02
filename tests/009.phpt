--TEST--
uopz_set_hook
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public function method($arg) {
		
	}
}

function bar() {}

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

try {
	uopz_set_hook(Bar::class, "none", function(){});
} catch (Throwable $t) {
	var_dump($t->getMessage());
}

var_dump(uopz_set_hook("bar", function(){
	var_dump("hook");
}));

bar();

var_dump(uopz_unset_hook("bar"));

bar();

var_dump(uopz_unset_hook("none"));
?>
--EXPECTF--
bool(true)
bool(true)
object(Foo)#2 (0) {
}
string(%d) "failed to set hook for %s::%s, the method is defined in %s"
string(%d) "failed to set hook for %s::%s, the method does not exist"
bool(true)
string(4) "hook"
bool(true)
bool(false)
