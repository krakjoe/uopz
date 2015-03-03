--TEST--
Test exit overload with parameters
--INI--
uopz.overloads=1
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_EXIT, function($status = null){
	var_dump($status);
});

exit(42);
echo "Hello World\n";
exit(84);
?>
--EXPECT--
int(42)
Hello World
int(84)
