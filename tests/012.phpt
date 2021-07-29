--TEST--
uopz_add_function
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
opcache.enable_cli=0
--FILE--
<?php
class Foo {
	public function exists() {}
}

uopz_add_function(Foo::class, "METHOD", /** doc **/ function(Type $type = null, $with = null, $args = null, ... $vars) : bool {
	return $this->priv(true);
});

uopz_add_function(Foo::class, "PRIV", /** doc **/ function(bool $arg){
	return $arg;
}, ZEND_ACC_PRIVATE); 

uopz_add_function(Foo::class, "STATICFUNCTION", /** doc **/ function(){
	static $a = [1,2,3];

	try {
		$a[] = 4;	
	} catch (Throwable $t) {

	} finally {
		return count($a) == 4;
	}
}, ZEND_ACC_STATIC);

uopz_add_function(Foo::class, "__construct", function() {
	var_dump("__construct");
});

uopz_add_function(Foo::class, "__destruct", function(){
	var_dump("__destruct");
});

uopz_add_function(Foo::class, "__clone", function() {
	var_dump("clone");

	return $this;
});

uopz_add_function(Foo::class, "__tostring", function(){
	return "string";
});

$foo = new Foo();

var_dump((string) $foo);

$x = clone $foo;

var_dump($foo->method());

var_dump(Foo::staticFunction());

try {
	var_dump($foo->priv(false));
} catch(Error $e) {
	var_dump($e->getMessage());	
}

try {
	uopz_add_function(Foo::class, "exists", function() {});
} catch(Exception $e) {
	var_dump($e->getMessage());
}

try {
	uopz_add_function("uopz_add_function", function(){});
} catch (Exception $e) {
	var_dump($e->getMessage());
}
?>
--EXPECTF--
string(%d) "__construct"
string(%d) "string"
string(%d) "clone"
bool(true)
bool(true)
string(%d) "Call to private method Foo::priv() from %s"
string(%d) "will not replace existing method %s::%s, use uopz_set_return instead"
string(%d) "will not replace existing function %s, use uopz_set_return instead"
string(%d) "__destruct"
string(%d) "__destruct"
