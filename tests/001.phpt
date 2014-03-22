--TEST--
Test exit overload
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_EXIT, function(){ return false; });

exit();
echo "ok";
uopz_overload(ZEND_EXIT, null);
exit();
echo "failed";
?>
--EXPECT--
ok
