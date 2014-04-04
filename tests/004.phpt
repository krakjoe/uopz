--TEST--
Test add interface
--INI--
uopz.overloads=1
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_ADD_INTERFACE, function($class, &$trait){
	if ($trait == "First") {
		$trait = "Second";
	}
});

interface First {
	public function myFirst();
}

interface Second {
	public function mySecond();
}

class My implements First {
	public function mySecond(){}
}

$my = new My();

var_dump($my instanceof Second);
?>
--EXPECTF--
bool(true)
