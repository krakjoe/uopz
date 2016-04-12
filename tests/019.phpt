--TEST--
uopz_set_property
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	private static $staticBar;
	private $bar;

	public static function testStaticBar() {
		return self::$staticBar;
	}

	public function testBar() {
		return $this->bar;
	}
}

$foo = new Foo();

uopz_set_property($foo, "bar", 10);

var_dump(uopz_get_property($foo, "bar"));

uopz_set_property(Foo::class, "staticBar", 100);

var_dump(uopz_get_property(Foo::class, "staticBar"));

var_dump($foo->testBar(), $foo->testStaticBar());
?>
--EXPECT--
int(10)
int(100)
int(10)
int(100)
