--TEST--
Test exit overload
--INI--
uopz.overloads=1
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_EXIT, function($status = null){
	return ZEND_USER_OPCODE_RETURN;
});

class Test {
	public function method() {
		exit(10);
	}
}

class Unit {
	public function test() {
		$test = new Test();
		$test->method();
		
		return true;
	} 
}
$unit = new Unit();
var_dump($unit->test());
uopz_overload(ZEND_EXIT, null);
var_dump($unit->test());
echo "failed";
?>
--EXPECT--
bool(true)
