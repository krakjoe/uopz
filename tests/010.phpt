--TEST--
Test delete
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_function("_My_strlen", function($string) {
	return strlen($string) * 5;
});

var_dump(uopz_delete("_my_strlen"));
?>
--EXPECT--
bool(true)


