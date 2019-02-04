--TEST--
uopz.exit enabled
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
uopz.exit=1
--FILE--
<?php
echo "OK";
exit();
echo "FAIL";
?>
--EXPECT--
OK
