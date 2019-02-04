--TEST--
Ensure uopz.allow_exit is respected
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
uopz.allow_exit=1
--FILE--
<?php
exit("exit\n");
echo "after exit\n";
--EXPECT--
exit
