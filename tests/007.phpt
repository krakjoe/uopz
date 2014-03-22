--TEST--
Test rename
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

uopz_rename("_my_strlen", "my_other_strlen");
uopz_rename(myClass::class, "first", "second");

var_dump(my_other_strlen("hello world"));

$my = new myClass();
var_dump(method_exists($my, "second"));
?>
--EXPECTF--
int(55)
bool(true)


