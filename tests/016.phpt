--TEST--
uopz_flags
--EXTENSIONS--
uopz
--SKIPIF--
<?php
uopz_allow_exit(true);
if (version_compare(PHP_VERSION, '7.4', '>=')
	&& function_exists('opcache_get_status')
	&& ($status = opcache_get_status())
	&& $status['opcache_enabled'])
{
	die('skip not for PHP 7.4+ with OPcache');
}
?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public function method() {}
}

$flags = uopz_flags(Foo::class, "method", PHP_INT_MAX);
var_dump((bool) (uopz_flags(
	Foo::class, "method", $flags | ZEND_ACC_PRIVATE) & ZEND_ACC_PRIVATE));
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
