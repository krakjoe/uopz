--TEST--
fetch class string ref no mock
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
var_dump($class::qux());
--EXPECTF--
Fatal error: Uncaught Error: Class name must be a valid object or a string in %s:2
Stack trace:
#0 {main}
  thrown in %s on line 2

