--TEST--
Test exit overload with exception
--INI--
uopz.overloads=1
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class ExitException extends RuntimeException {}

uopz_overload(ZEND_EXIT, function($status = null){
	throw new ExitException("exit. STATUS=$status");
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
		try {
			$test->method();
		} catch(ExitException $e) {
			echo "exit caught";
		}
		
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
exit caught
bool(true)
