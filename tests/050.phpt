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
	return ZEND_USER_OPCODE_RETURN;
});

class Test {
	public function method($status) {
		exit($status);
	}
}

class Unit {
	public function test($status) {
		$test = new Test();
		$test->method($status);
		
		return true;
	} 
}
$unit = new Unit();
$unit->test(42);
echo "Hello World\n";
$unit->test(84);
?>
--EXPECT--
int(42)
Hello World
int(84)
