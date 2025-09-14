--TEST--
uopz_get_hook
--EXTENSIONS--
uopz
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

var_dump(uopz_get_hook(Foo::class, "method"));

uopz_set_hook("bar", function(){});

var_dump(uopz_get_hook("bar"));

var_dump(uopz_get_hook("none"));

var_dump(uopz_get_hook(DateTime::class, "__construct"));
?>
--EXPECTF--
bool(true)
object(Closure)#%d (%d) {%A
  ["parameter"]=>
  array(1) {
    ["$arg"]=>
    string(10) "<required>"
  }
}
object(Closure)#%d (%d) {%A
}
NULL
NULL
