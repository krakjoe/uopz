--TEST--
Test redefine
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
define("MY", 1);
uopz_redefine("MY", 100);
var_dump(MY);
?>
--EXPECT--
int(100)


