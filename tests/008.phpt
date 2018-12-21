--TEST--
uopz_set_static
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php
class Foo {
	public function method() {
		static $vars = [
			1,2,3,4,5];

		$vars[] = 6;
	}

	public function nostatics() {}
}

function nostatics() {}

$foo = new Foo();

$foo->method();

var_dump(uopz_get_static(Foo::class, "method"));

uopz_set_static(Foo::class, "method", [
	"vars" => []
]);

$foo->method();

var_dump(uopz_get_static(Foo::class, "method"));

try {
	uopz_set_static(Foo::class, "none", []);
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_set_static("none", []);
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_set_static("phpversion", []);
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_set_static(DateTime::class, "__construct", []);
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_set_static(Foo::class, "nostatics", []);
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_set_static("nostatics", []);
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECTF--
array(1) {
  ["vars"]=>
  array(6) {
    [0]=>
    int(1)
    [1]=>
    int(2)
    [2]=>
    int(3)
    [3]=>
    int(4)
    [4]=>
    int(5)
    [5]=>
    int(6)
  }
}
array(1) {
  ["vars"]=>
  array(1) {
    [0]=>
    int(6)
  }
}
string(%d) "failed to set statics in method %s::%s, it does not exist"
string(%d) "failed to set statics in function %s, it does not exist"
string(%d) "failed to set statics in internal function %s"
string(%d) "failed to set statics in internal method %s::%s"
string(%d) "failed to set statics in method %s::%s, no statics declared"
string(%d) "failed to set statics in function %s, no statics declared"
