--TEST--
Test bug in finding functions
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_compose("A", [], [
	"__construct" => [
		ZEND_ACC_PRIVATE => function() {
			var_dump($this);
		}
	],
	"getInstance" => [
		ZEND_ACC_PUBLIC | ZEND_ACC_STATIC => function() {
			/* this doesn't fail because we cannot 
				run verify abstract class on composed classes */
			return new self();
		}
	],
	"test" => [ZEND_ACC_PUBLIC|ZEND_ACC_STATIC => function() {
		return 5;
	}]
]);

$original = uopz_copy("A", "test");
uopz_function("A", "test", function() use ($original) {
	return 5 * $original();
}, ZEND_ACC_PUBLIC);

$a = A::getInstance();
var_dump($a->test());
?>
--EXPECTF--
object(A)#%d (0) {
}
int(25)

