--TEST--
uopz_unset_return
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function bar() {
		return false;	
	}
}

function bar() {
	return false;
}

var_dump(uopz_set_return(Foo::class, "bar", true));

$foo = new Foo();

var_dump($foo->bar());

var_dump(uopz_unset_return(Foo::class, "bar"));

var_dump($foo->bar());

var_dump(uopz_unset_return(Foo::class, "nope"));

var_dump(uopz_set_return("bar", function(){
	return true;
}, true));

var_dump(uopz_get_return("bar"));
var_dump(bar());

var_dump(uopz_unset_return("bar"));

var_dump(uopz_get_return(DateTime::class, "__construct"));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(false)
bool(false)
bool(true)
object(Closure)#2 (0) {
}
bool(true)
bool(true)
NULL
