--TEST--
uopz_set_mock
--SKIPIF--
<?php include("skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Bar {
	const THING = 5;

	public static function thing() {
		return 10;
	}
}

uopz_set_mock("Foo", "Bar");

var_dump($foo = new Foo);

var_dump(FOO::THING);

var_dump($foo::THING);

var_dump(Foo::thing());

$foo = "Foo";

var_dump(new $foo);

class Qux {
	const THING = 50;

	public static function thing() {
		return 20;
	}
}

uopz_set_mock(Foo::class, new Qux);

var_dump($foo = new Foo);

var_dump(FOO::THING);

var_dump($foo::THING);

var_dump(Foo::thing());

interface IFaceMock {
	
}

uopz_set_mock(IFace::class, IFaceMock::class);

class Interfacing implements IFace {}

var_dump(new Interfacing);

trait ITraitMock {

}

uopz_set_mock(ITrait::class, ITraitMock::class);

class Traiting {
	use ITrait;
}

var_dump(new Traiting());
?>
--EXPECT--
object(Bar)#1 (0) {
}
int(5)
int(5)
int(10)
object(Bar)#1 (0) {
}
object(Qux)#1 (0) {
}
int(50)
int(50)
int(20)
object(Interfacing)#2 (0) {
}
object(Traiting)#2 (0) {
}
