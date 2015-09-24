--TEST--
Test instanceof
--INI--
uopz.overloads=1
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_INSTANCEOF, function($object, &$class){
	var_dump($object, $class);
});

interface First {
	public function myFirst();
}

interface Second {
	public function mySecond();
}

class My implements Second {
	public function mySecond(){}
}

$my = new My();

var_dump($my instanceof Second);
?>
--EXPECTF--
object(My)#%d (0) {
}
string(6) "Second"
bool(true)
