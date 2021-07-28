--TEST--
uopz_extend final class
--EXTENSIONS--
uopz
--FILE--
<?php
class Foo {}
class Bar {}

uopz_extend(Foo::class, Bar::class);

if (new Foo instanceof Bar) {
	echo "OK";
}
?>
--EXPECT--
OK
