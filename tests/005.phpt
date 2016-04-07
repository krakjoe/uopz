--TEST--
Test uopz_set_return with more magic than is reasonable
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
uopz_set_return("strtotime", function($string){
	return $string;
}, true);

var_dump(strtotime("hai"));

uopz_unset_return("strtotime");

var_dump(strtotime("hai"));

class Foo {
	public function bar($thing) {
		return true;
	}
}

uopz_set_return(Foo::class, "bar", function($thing){
	return $thing;
}, true);

$foo = new Foo();

var_dump($foo->bar("what have I done"));

uopz_unset_return(Foo::class, "bar");

var_dump($foo->bar("darkest magic"));
?>
--EXPECT--
string(3) "hai"
bool(false)
string(16) "what have I done"
bool(true)

