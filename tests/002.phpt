--TEST--
uopz_get_return
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

	public function bazQuux() {
		return false;
	}
}

var_dump(uopz_set_return(Foo::class, "bar", true));

var_dump(uopz_get_return(Foo::class, "bar"));

var_dump(uopz_set_return(Foo::class, "bar", function(){}));

var_dump(uopz_get_return(Foo::class, "bar"));

var_dump(uopz_get_return(Foo::class, "nope"));

var_dump(uopz_set_return(Foo::class, "bazQuux", true));

var_dump(uopz_get_return(Foo::class, "bazQuux"));

var_dump(uopz_set_return(Foo::class, "bazQuux", function(){}));

var_dump(uopz_get_return(Foo::class, "bazQuux"));
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
object(Closure)#%d (%d) {%A
}
NULL
bool(true)
bool(true)
bool(true)
object(Closure)#%d (%d) {%A
}
