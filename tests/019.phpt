--TEST--
uopz_set_property/uopz_get_property
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
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

echo "----------------------\n";
$foo = new Foo();

uopz_set_property($foo, "bar", 10);

var_dump(uopz_get_property($foo, "bar"));
echo "-----------------------\n\n";

echo "----------------------\n";
uopz_set_property(Foo::class, "staticBar", 100);

var_dump(uopz_get_property(Foo::class, "staticBar"));

var_dump($foo->testBar(), $foo->testStaticBar());

try {
	uopz_set_property(Foo::class, "staticQux", 10);
} catch (RuntimeException $ex) {
	var_dump($ex->getMessage());
}
echo "-----------------------\n\n";

echo "----------------------\n";
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

#[AllowDynamicProperties]
class Bill extends Baz {}

$bill = new Bill;

uopz_set_property($bill, "bar", 100);

var_dump($bill->foo(), uopz_get_property($bill, "bar"));

uopz_set_property($bill, "qux", 1000);

var_dump($bill->qux, uopz_get_property($bill, "qux"));

var_dump(@uopz_get_property($bill, "none"));
echo "-----------------------\n\n";
?>
--EXPECTF--
----------------------
int(10)
-----------------------

----------------------
int(100)
int(10)
int(100)
string(%d) "cannot set non-existent static property %s::%s"
-----------------------

----------------------
int(10)
int(10)
int(100)
int(100)
int(1000)
int(1000)
NULL
-----------------------
