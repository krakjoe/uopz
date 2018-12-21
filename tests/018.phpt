--TEST--
uopz_undefine
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	const BAR = 1;
	const QUX = 2;
}

const UOPZ_DEFINED = "DEFINED";

var_dump(uopz_undefine("UOPZ_DEFINED"));

var_dump(@constant("UOPZ_DEFINED"));

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
NULL
int(1)
int(1)
bool(false)
int(0)
OK
