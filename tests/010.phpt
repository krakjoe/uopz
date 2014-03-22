--TEST--
Test delete
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function _my_strlen($string) {
	return strlen($string) * 5;
}

class MyClass {
	public function first() {
		
	}
}

uopz_delete("_my_strlen");
uopz_delete(myClass::class, "first");

var_dump(function_exists("_my_strlen"));

$my = new myClass();
var_dump(method_exists($my, "first"));
?>
--EXPECT--
bool(false)
bool(false)


