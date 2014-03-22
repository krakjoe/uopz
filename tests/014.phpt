--TEST--
Test compose
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class My {
	public function is() {}
}

interface IMy {
	public function is();
}

trait myTrait {
	public function myOtherTrait() {
		
	}
}

uopz_compose("CreatedClass", [
	My::class, 
	IMy::class, 
	myTrait::class
]);

if (class_exists("CreatedClass")) {
	$test = new CreatedClass();

	var_dump(
		class_uses($test),
		class_parents($test),
		class_implements($test));
}
?>
--EXPECT--
array(1) {
  ["myTrait"]=>
  string(7) "myTrait"
}
array(1) {
  ["My"]=>
  string(2) "My"
}
array(1) {
  ["IMy"]=>
  string(3) "IMy"
}
