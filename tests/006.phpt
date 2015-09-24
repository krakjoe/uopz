--TEST--
Test throw
--INI--
uopz.overloads=1
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_overload(ZEND_THROW, function($exception){
	if (!$exception instanceof RuntimeException) {
		/* we can throw from here now */
		throw new RuntimeException(
			"additional", ZEND_THROW, $exception);
	}
});

throw new Exception("basic");
?>
--EXPECTF--
Fatal error: Uncaught Exception: basic in %s:%d
Stack trace:
#0 {main}

Next RuntimeException: additional in %s:%d
Stack trace:
#0 %s(%d): {closure}(Object(Exception), NULL)
#1 {main}
  thrown in %s on line %d


