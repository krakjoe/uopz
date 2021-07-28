--TEST--
uopz_undefine with namespaced constants does not delete constant
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php

uopz_redefine('Test\UNDEF', 123);

uopz_undefine('Test\UNDEF');

var_dump(Test\UNDEF);
?>
--EXPECTF--
Fatal error: Uncaught Error: Undefined constant %s in %s:7
Stack trace:
#0 {main}
  thrown in %s on line 7

