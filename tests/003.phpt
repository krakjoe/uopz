--TEST--
uopz_unset_return
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
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
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(false)
bool(false)
bool(true)
object(Closure)#%d (%d) {
%A}
bool(true)
bool(true)
NULL
