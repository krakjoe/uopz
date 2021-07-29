--TEST--
uopz_flags
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
opcache.enable_cli=0
--FILE--
<?php
class Foo {
	public function method() {}
}

var_dump((bool) (uopz_flags(Foo::class, "method", ZEND_ACC_PRIVATE) & ZEND_ACC_PRIVATE));
var_dump((bool) (uopz_flags(Foo::class, "method", PHP_INT_MAX) & ZEND_ACC_PRIVATE));

var_dump((bool) (uopz_flags(Foo::class, '') & ZEND_ACC_FINAL));

try {
	uopz_flags(Foo::class, '', ZEND_ACC_PRIVATE);
} catch (Exception $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_flags(Foo::class, '', ZEND_ACC_STATIC);
} catch (Exception $ex) {
	var_dump($ex->getMessage());
}

uopz_flags(Foo::class, '', ZEND_ACC_FINAL);

$reflector = new ReflectionClass(Foo::class);

var_dump($reflector->isFinal());

try {
	uopz_flags(Foo::class, "none", ZEND_ACC_PUBLIC);
} catch (Exception $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_flags("none", ZEND_ACC_STATIC);
} catch (Exception $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECTF--
bool(false)
bool(true)
bool(false)
string(%d) "attempt to set public, private or protected on class entry %s, not allowed"
string(%d) "attempt to set static on class entry %s, not allowed"
bool(true)
string(%d) "failed to set or get flags of method %s::%s, it does not exist"
string(%d) "failed to set or get flags of function %s, it does not exist"
