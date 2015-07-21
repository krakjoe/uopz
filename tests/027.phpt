--TEST--
Test use original function
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function mystrlen(string $string) {
	return strlen($string);
}

$mystrlen = uopz_copy('mystrlen');
uopz_function('mystrlen', function ($string) use($mystrlen) {
	return $mystrlen($string) * 2;
});
mystrlen("Hello World");
?>
--EXPECTF--
int(22)

