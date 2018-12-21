--TEST--
uopz_del_function
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function exists() {}
}

var_dump(uopz_add_function(Foo::class, "method", function(){
	return true;
}));

$foo = new Foo();

var_dump($foo->method());

var_dump(uopz_del_function(Foo::class, "method"));

try {
	$foo->method();
} catch(Throwable $e) {
	var_dump($e->getMessage());	
}

try {
	uopz_del_function(Foo::class, "exists");
} catch (Throwable $t) {
	var_dump($t->getMessage());
}

try {
	uopz_del_function("phpversion");
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
string(%d) "Call to undefined method Foo::method()"
string(%d) "cannot delete method %s::%s, it was not added by uopz"
string(%d) "cannot delete function %s, it was not added by uopz"
