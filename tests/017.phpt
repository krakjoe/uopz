--TEST--
uopz_redefine
--SKIPIF--
<?php include("skipif.inc") ?>
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
