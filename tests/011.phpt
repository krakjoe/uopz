--TEST--
Test undefine/redefine
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
define ("MY", 1);
uopz_undefine("MY");
var_dump(defined("MY"));
class Upper {
	const NUM = 10;
}
class Lower extends Upper {}
uopz_undefine("Lower", "NUM");
var_dump(@constant("Lower::NUM"),
		 @constant("Upper::NUM"));
?>
--EXPECT--
bool(false)
NULL
NULL



