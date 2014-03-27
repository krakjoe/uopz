--TEST--
Test use original function
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
$strlen = uopz_copy('strlen');
uopz_function('strlen', function ($string) use($strlen) {
	return $strlen($string) * 2;
});
var_dump(strlen("Hello World"));
uopz_restore('strlen');
var_dump(strlen("Hello World"));
?>
--EXPECTF--
int(22)
int(11)

