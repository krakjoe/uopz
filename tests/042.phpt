--TEST--
Ensure uopz.allow_exit can be overridden
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
uopz.disable=0
uopz.allow_exit=1
--FILE--
<?php
uopz_allow_exit(false);
exit("exit\n");
echo "after exit\n";
--EXPECT--
after exit
