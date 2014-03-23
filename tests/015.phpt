--TEST--
Test override
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class A {

	private $data = 'PRIVATE!'; 
	
	public function foo () { 
		return __METHOD__;
	}
}

class B {

	public function bar () {
		return $this->data;
	}
}

uopz_compose(
	C::class, [
		A::class, 
		B::class
	]
);

uopz_function("whatever", function(){
	return "whatever";
});

uopz_function(C::class, "foo", function(){
	return "overriden";
});

uopz_function(C::class, "FoF", function(){
	return true;
});

$c = new C();
var_dump($c->foo(), whatever(), $c->fof());

--EXPECT--
string(9) "overriden"
string(8) "whatever"
bool(true)

