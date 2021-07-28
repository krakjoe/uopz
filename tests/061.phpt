--TEST--
uopz_extend invalid parent
--EXTENSIONS--
uopz
--FILE--
<?php
class Foo {}
class Bar extends Foo {}

uopz_extend(Bar::class, Foo::class);
?>
--EXPECTF--
Fatal error: Uncaught RuntimeException: the class provided (Bar) already extends Foo in %s:5
Stack trace:
#0 %s(5): uopz_extend('Bar', 'Foo')
#1 {main}
  thrown in %s on line 5

