--TEST--
Test call caching processed
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class C1 {
	public static function method() {
		return 1;
	}
}

class C2 {
	public function method() {
		return C1::method();
	}
}

$c = new C2;

uopz_function('C1', 'method', function() { return false; });
var_dump($c->method());

uopz_function('C1', 'method', function() { return true; }, ZEND_ACC_STATIC);
var_dump($c->method());

uopz_restore('C1', 'method');
var_dump($c->method());
?>
--EXPECT--
bool(false)
bool(true)
int(1)
