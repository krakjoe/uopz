--TEST--
fetch class undef class no mock
--EXTENSIONS--
uopz
--INI--
error_reporting=32759
uopz.disable=0
--FILE--
<?php
var_dump(@$class::qux());
?>
--EXPECTF--
Fatal error: Uncaught Error: Class name must be a valid object or a string in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d

