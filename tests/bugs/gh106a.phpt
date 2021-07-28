--TEST--
uopz.exit disabled
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
uopz.exit=0
--FILE--
<?php
echo "OK\n";
exit();
echo "OK";
?>
--EXPECT--
OK
OK
