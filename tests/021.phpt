--TEST--
uopz.disabled
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=1
--FILE--
<?php
uopz_set_return();
?>
--EXPECTF--
Fatal error: Uncaught %s: uopz is disabled by configuration (uopz.disable) in %s:2
Stack trace:
#0 %s(2): uopz_set_return()
#1 {main}
  thrown in %s on line 2
