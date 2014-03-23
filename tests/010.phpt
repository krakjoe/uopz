--TEST--
Test delete
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function _MY_strlen($string) {
	return Strlen($string) * 5;
}

class MyClass {
	public function First() {
		
	}
}

uopz_delete("_my_strlen");
uopz_delete("myClass", "first");

var_dump(function_exists("_my_strlen"));

$my = new myClass();
var_dump(method_exists($my, "first"));
?>
--EXPECT--
bool(false)
bool(false)


