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
	"C", array(
		"A",
		"B"
	)
);

uopz_function("whatever", function(){
	return "whatever";
});

uopz_function("C", "foo", function(){
	return "overriden";
});

uopz_function("C", "FoF", function(){
	return true;
});

uopz_function("C", "myClosure", function(){
	return __FUNCTION__;
});

$c = new C();
var_dump($c->foo(), whatever(), $c->fof(), C::myClosure());

--EXPECT--
string(9) "overriden"
string(8) "whatever"
bool(true)
string(9) "{closure}"

