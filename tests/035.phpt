--TEST--
Test fetch class and compose
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
/* could be a trait, if you like that sort of thing ... */
class TestBundle {

	public function mine() {
		return $this->arg;
	}	
	
	protected $arg;
}

/* tiny amount of voodoo ... */
uopz_overload(ZEND_FETCH_CLASS, function(&$class) {
	switch ($class) {
		case "test": {
			uopz_compose($class, ["TestBundle"], function($arg) {
				$this->arg = $arg;
			});
		} break;
	}
});

/* we will compose test class on the fly */
$test = new test(new stdClass());
var_dump(
	$test, 
	class_parents($test));
?>
--EXPECTF--
object(test)#%d (%d) {
  ["arg":protected]=>
  object(stdClass)#%d (0) {
  }
}
array(1) {
  ["TestBundle"]=>
  string(10) "TestBundle"
}



