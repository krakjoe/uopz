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
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
string(38) "Call to undefined method Foo::method()"
string(58) "cannot delete method Foo::exists, it was not added by uopz"

