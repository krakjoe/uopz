--TEST--
uopz_undefine
--SKIPIF--
<?php
uopz_allow_exit(true);
$protect = extension_loaded("Zend OPcache")
	&& ($conf = opcache_get_configuration()["directives"])
	&& array_key_exists("opcache.protect_memory", $conf)
	&&  $conf["opcache.protect_memory"];
if ($protect) die("xfail known issues with constant redefinition; see #151");
?>
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	const BAR = 1;
	const QUX = 2;
}

const UOPZ_DEFINED = "DEFINED";

var_dump(uopz_undefine("UOPZ_DEFINED"));

var_dump(defined("UOPZ_DEFINED"));

var_dump(FOO::BAR);

uopz_undefine(Foo::class, "BAR");

$reflector = new ReflectionClass(Foo::class);

var_dump(count($reflector->getConstants()));

var_dump(uopz_undefine(Foo::class, "NONE"));

uopz_undefine(Foo::class, "QUX");

var_dump(count($reflector->getConstants()));

try {
	uopz_undefine("PHP_VERSION");
} catch (RuntimeException $ex) {
	echo "OK\n";
}
?>
--EXPECT--
bool(true)
bool(false)
int(1)
int(1)
bool(false)
int(0)
OK
