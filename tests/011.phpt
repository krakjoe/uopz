--TEST--
Test undefine
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
define ("MY", 1);
uopz_undefine("MY");
var_dump(defined("MY"));
?>
--EXPECT--
bool(false)


