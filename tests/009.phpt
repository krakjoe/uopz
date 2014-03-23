--TEST--
Test alias
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function _my_Strlen($string) {
	return Strlen($string) * 5;
}

class MyClass {
	public function First() {
		
	}
}

uopz_alias("_my_strlen", "my_other_strlen");
uopz_alias(myClass::class, "first", "second");
var_dump(my_other_strlen("hello world"), 
		 function_exists("_my_strlen"));

$my = new myClass();
var_dump(method_exists($my, "second"), 
		 method_exists($my, "first"));

uopz_alias("strlen", "strlen_stub");
var_dump(function_exists("strlen_stub"));
var_dump(strlen_stub("hello"));
?>
--EXPECTF--
int(55)
bool(true)
bool(true)
bool(true)
bool(true)
int(5)
