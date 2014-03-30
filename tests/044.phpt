--TEST--
Test overload error conditions for bad handlers
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
try {
	uopz_overload(ZEND_NEW, function($other, $two, $three){});
} catch (InvalidArgumentException $ex) {
	printf("%s\n\n", (string)$ex);
}

try {
	uopz_overload(ZEND_EXIT, function($op1){});
} catch (InvalidArgumentException $ex) {
		printf("%s\n\n", (string)$ex);
}

try {
	uopz_overload(ZEND_INSTANCEOF, function($op1){});
} catch (InvalidArgumentException $ex) {
		printf("%s\n\n", (string)$ex);
}

try {
	uopz_overload(ZEND_ADD_TRAIT, function($op1){});
} catch (InvalidArgumentException $ex) {
		printf("%s\n\n", (string)$ex);
}

try {
	uopz_overload(ZEND_ADD_INTERFACE, function($op1){});
} catch (InvalidArgumentException $ex) {
		printf("%s\n\n", (string)$ex);
}

try {
	uopz_overload(ZEND_FETCH_CLASS, function(){});
} catch (InvalidArgumentException $ex) {
		printf("%s\n\n", (string)$ex);
}

try {
	uopz_overload(ZEND_THROW, function(){});
} catch (InvalidArgumentException $ex) {
		printf("%s\n\n", (string)$ex);
}
?>
--EXPECTF--
exception '%s' with message 'invalid handler for ZEND_NEW, expected function(class)' in %s:%d
Stack trace:
#0 %s(3): uopz_overload(68, Object(Closure))
#1 {main}

exception '%s' with message 'invalid handler for ZEND_EXIT, expected function(void)' in %s:%d
Stack trace:
#0 %s(9): uopz_overload(79, Object(Closure))
#1 {main}

exception '%s' with message 'invalid handler for ZEND_INSTANCEOF, expected function(object, class)' in %s:%d
Stack trace:
#0 %s(15): uopz_overload(138, Object(Closure))
#1 {main}

exception '%s' with message 'invalid handler for ZEND_ADD_TRAIT, expected function(class, trait)' in %s:%d
Stack trace:
#0 %s(21): uopz_overload(154, Object(Closure))
#1 {main}

exception '%s' with message 'invalid handler for ZEND_ADD_INTERFACE, expected function(class, interface)' in %s:%d
Stack trace:
#0 %s(27): uopz_overload(144, Object(Closure))
#1 {main}

exception '%s' with message 'invalid handler for ZEND_FETCH_CLASS, expected function(class)' in %s:%d
Stack trace:
#0 %s(33): uopz_overload(109, Object(Closure))
#1 {main}

exception '%s' with message 'invalid handler for ZEND_THROW, expected function(exception)' in %s:%d
Stack trace:
#0 %s(39): uopz_overload(108, Object(Closure))
#1 {main}

