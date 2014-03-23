--TEST--
Test add trait
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_ADD_TRAIT, function($class, &$trait){
	if ($trait == "First") {
		$trait = "Second";
	}
});

trait First {
	public function myFirst() {}
}

trait Second {
	public function mySecond() {}
}

class My {
	use First;
	
}

$my = new My();

var_dump(method_exists($my, "mySecond"));
?>
--EXPECTF--
bool(true)
