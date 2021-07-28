--TEST--
uopz_extend invalid interfaces
--EXTENSIONS--
uopz
--FILE--
<?php
interface Foo {}
class Bar {}

uopz_extend(Foo::class, Bar::class);
?>
--EXPECTF--
Fatal error: Uncaught RuntimeException: the interface provided (Foo) cannot extend Bar, because Bar is not an interface in %s:5
Stack trace:
#0 %s(5): uopz_extend('Foo', 'Bar')
#1 {main}
  thrown in %s on line 5
