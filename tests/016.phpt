--TEST--
Test magic methods
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class A {
	protected $test = "value";
	
	protected function other() {
		return $this;
	}
}

uopz_compose(C::class, [A::class], function(){
	echo "do not execute\n";
});

uopz_function(C::class, "__toString", function(){
	return "converting to string\n";
});

uopz_function(C::class, "__construct", function(){
	var_dump($this);
});

uopz_function(C::class, "__callStatic", function($method, $args) {
	echo "calling static method\n";
	var_dump($method, $args);
});

uopz_function(C::class, "__sleep", function(){
	echo "sleeping\n";
	return [];
});

uopz_function(C::class, "__wakeup", function(){
	echo "waking\n";
	return [];
});

/* implementing serializable on the fly */
uopz_function(C::class, "serialize", function(){
	echo "serializing\n";
	return "hello world";
});

uopz_function(C::class, "unserialize", function($data){
	echo "unserializing\n";
	var_dump($data);
});

uopz_implement(C::class, Serializable::class);


$c = new C();
echo (string)$c;
C::none("argument");
$s = serialize($c);
$u = unserialize($s);
?>
--EXPECTF--
object(C)#1 (1) {
  ["test":protected]=>
  string(5) "value"
}
converting to string
calling static method
string(4) "none"
array(1) {
  [0]=>
  string(8) "argument"
}
serializing
unserializing
string(11) "hello world"
