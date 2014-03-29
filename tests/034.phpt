--TEST--
Test fetch class overload
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_FETCH_CLASS, function(&$class) {
	if ($class == "other") {
		$class = "mine";
	}
});

class mine {}
class test extends other {}

$test = new test();
var_dump(
	$test, 
	class_parents($test));
?>
--EXPECTF--
object(test)#%d (%d) {
}
array(1) {
  ["mine"]=>
  string(4) "mine"
}


