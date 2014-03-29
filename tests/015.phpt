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

uopz_compose("B", ["A"], [
	"__construct" => function() { 
		var_dump($this, 	
				 $this->data); 
	},
	"foo" => function() {
		return "overriden";
	},
	"FoF" => function() {
		return true;
	},
	"myClosure" => [
		ZEND_ACC_STATIC | ZEND_ACC_PUBLIC => function() {
			return __FUNCTION__;
		}
	]
]);

uopz_function("whatever", function(){
	return "whatever";
});

$b = new B();
var_dump($b->foo(), whatever(), $b->fof(), B::myClosure());

--EXPECTF--
object(B)#%d (1) {
  ["data":protected]=>
  string(9) "protected"
}
string(9) "protected"
string(9) "overriden"
string(8) "whatever"
bool(true)
string(9) "{closure}"
