--TEST--
Test complicated construction of classes
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
abstract class Hood {
	abstract public function implement();
}

uopz_compose("A", ["Hood"], [
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
	"abstracted" => [ZEND_ACC_ABSTRACT => function() {
		/* you can declare abstracts in this way 
			function body will be ignored, still cannot call it ... */
	}],
	/* all of [this isn't required] all the time ... */
	"implement" => function(){}
]);

$a = A::getInstance();
$a->abstracted();
?>
--EXPECTF--
object(A)#%d (0) {
}

Fatal error: Cannot call abstract method %s() in %s on line %d
