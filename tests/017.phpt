--TEST--
uopz_redefine
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
}

var_dump(FOO::BAR);

uopz_redefine(Foo::class, "BAR", 2);

var_dump(FOO::BAR);

uopz_redefine(Foo::class, "QUX", 3);

var_dump(FOO::QUX);

uopz_redefine("UOPZ_REDEFINED", 4);

var_dump(UOPZ_REDEFINED);

uopz_redefine("UOPZ_REDEFINED", 5);

var_dump(UOPZ_REDEFINED);

try {
	uopz_redefine("PHP_VERSION", 10);
} catch (RuntimeException $t) {
	echo "OK\n";
}
?>
--EXPECT--
int(1)
int(2)
int(3)
int(4)
int(5)
OK
