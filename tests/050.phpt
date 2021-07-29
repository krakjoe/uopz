--TEST--
type lists
--EXTENSIONS--
uopz
--INI--
opcache.enable_cli=0
--FILE--
<?php
declare(strict_types=1);

class Foo {}
class Bar {}
class Baz {}

uopz_add_function(Foo::class, 'bar', function(Bar|Baz $barOrBaz) : Bar|Baz{
	return $barOrBaz;
});

$foo = new Foo;

var_dump($foo->bar(new Bar), $foo->bar(new Baz));
?>
--EXPECTF--
object(Bar)#%d (0) {
}
object(Baz)#%d (0) {
}

