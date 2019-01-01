--TEST--
fetch class constant non existent class
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php
var_dump(Test::none);
--EXPECTF--
Fatal error: Uncaught Error: Class 'Test' not found in %s:2
Stack trace:
#0 {main}
  thrown in %s on line 2
