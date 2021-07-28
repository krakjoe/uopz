--TEST--
uopz_extend invalid traits
--EXTENSIONS--
uopz
--FILE--
<?php
trait Foo {}
class Bar {}

uopz_extend(Foo::class, Bar::class);
?>
--EXPECTF--
Fatal error: Uncaught RuntimeException: the trait provided (Foo) cannot extend Bar, because Bar is not a trait in %s:5
Stack trace:
#0 %s(5): uopz_extend('Foo', 'Bar')
#1 {main}
  thrown in %s on line 5
