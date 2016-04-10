--TEST--
uopz_add_function
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function exists() {}
}

uopz_add_function(Foo::class, "METHOD", function(){
	return $this->priv();
});

uopz_add_function(Foo::class, "PRIV", function(){
	return true;
}, ZEND_ACC_PRIVATE); 

uopz_add_function(Foo::class, "STATICFUNCTION", function(){
	return true;
}, ZEND_ACC_STATIC);

$foo = new Foo();

var_dump($foo->method());

var_dump(Foo::staticFunction());

try {
	var_dump($foo->priv());
} catch(Error $e) {
	var_dump($e->getMessage());	
}

try {
	uopz_add_function(Foo::class, "exists", function() {});
} catch(Exception $e) {
	var_dump($e->getMessage());
}
?>
--EXPECT--
bool(true)
bool(true)
string(50) "Call to private method Foo::priv() from context ''"
string(69) "uopz will not replace existing functions, use uopz_set_return instead"
