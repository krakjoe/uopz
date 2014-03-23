--TEST--
Test extend
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class My {
	
}

trait MyTrait {
	public function is() {
		
	}
}

uopz_extend("My", "MyTrait");

$my = new My();

var_dump(method_exists($my, "is"), 
		 class_parents($my), 
		 class_uses($my));
?>
--EXPECT--
bool(true)
array(0) {
}
array(1) {
  ["MyTrait"]=>
  string(7) "MyTrait"
}



