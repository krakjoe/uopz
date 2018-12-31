--TEST--
init method call non object object
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
$foo = "string";

var_dump($foo->method());
?>
--EXPECTF--
Fatal error: Uncaught Error: Call to a member function method() on string in %s:4
Stack trace:
#0 {main}
  thrown in %s on line 4
