--TEST--
uopz.exit enabled
--EXTENSIONS--
uopz
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
