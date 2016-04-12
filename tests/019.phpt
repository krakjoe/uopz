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

class Bar {
	private static $bar;

	public function foo() {
		return self::$bar;
	}
}

class Qux extends Bar {}

$qux = new Qux;

uopz_set_property(Qux::class, "bar", 10);

var_dump($qux->foo(), uopz_get_property(Qux::class, "bar"));

class Baz {
	private $bar;

	public function foo() {
		return $this->bar;
	}
}

class Bill extends Baz {}

$bill = new Bill;

uopz_set_property($bill, "bar", 100);

var_dump($bill->foo(), uopz_get_property($bill, "bar"));
?>
--EXPECT--
int(10)
int(100)
int(10)
int(100)
int(10)
int(10)
int(100)
int(100)
