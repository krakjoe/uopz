--TEST--
Test override
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class A {
	protected $data = 'protected'; 
	
	public function foo () { 
		return __METHOD__;
	}
}

uopz_compose("B", array("A"), function() { 
	var_dump($this, 	
			 $this->data); 
});

uopz_function("whatever", function(){
	return "whatever";
});

uopz_function("B", "foo", function(){
	return "overriden";
});

uopz_function("B", "FoF", function(){
	return true;
});

uopz_function("B", "myClosure", function(){
	return __FUNCTION__;
}, true);

$b = new B();
var_dump($b->foo(), whatever(), $b->fof(), B::myClosure());

--EXPECT--
object(B)#1 (1) {
  ["data":protected]=>
  string(9) "protected"
}
string(9) "protected"
string(9) "overriden"
string(8) "whatever"
bool(true)
string(9) "{closure}"
