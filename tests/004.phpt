--TEST--
uopz_set_mock
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	static $prop = 1;

	public static function method() {
		return -1;	
	}
}

class Bar {
	static $prop = 2;
}

uopz_set_mock(Foo::class, Bar::class);

var_dump(new Foo(), Foo::$prop);

var_dump(Foo::method());

uopz_unset_mock(Foo::class);

uopz_set_mock(Bar::class, new Foo);

var_dump(new Bar());

uopz_set_mock(Foo::class, DoesntExist::class);
try {
    var_dump(new Foo);
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECTF--
object(Bar)#%d (0) {
}
int(1)
int(-1)
object(Foo)#%d (0) {
}
Class "DoesntExist" not found
