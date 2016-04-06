--TEST--
Test set return
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_set_return("strtotime", 0);

var_dump(strtotime("junk"));

uopz_unset_return("strtotime");

var_dump(strtotime("junk"));

class Foo {
	public function bar() {
		return false;
	}
}

$foo = new Foo();

uopz_set_return(Foo::class, "bar", 0);

var_dump($foo->bar());

uopz_unset_return(Foo::class, "bar");

var_dump($foo->bar());

class Bar extends Foo {

}

uopz_set_return(Foo::class, "bar", 0);

$bar = new Bar();

var_dump($bar->bar());

uopz_unset_return(Foo::class, "bar");

var_dump($bar->bar());

uopz_set_return(Exception::class, "__toString", "0");

$ex = new Exception("");

var_dump((string) $ex);
?>
--EXPECTF--
int(0)
bool(false)
int(0)
bool(false)
int(0)
bool(false)
string(1) "0"
