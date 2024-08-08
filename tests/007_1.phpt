--TEST--
uopz_get_static
--EXTENSIONS--
uopz
--SKIPIF--
<?php
uopz_allow_exit(true);
if (version_compare(PHP_VERSION, "8.3.0") < 0) die("skip only for PHP >= 8.3.0");
?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public function method() {
		static $vars = [1,2,3,4,5];
		static $bar = FOO;

		$vars[] = 6;
	}

	public function nostatics() {}
}
const FOO = "bar";

function nostatics() {}

$foo = new Foo();

var_dump(uopz_get_static(Foo::class, "method"));

$foo->method();

var_dump(uopz_get_static(Foo::class, "method"));

try {
	uopz_get_static(Foo::class, "none");
} catch (RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_get_static("none");
} catch (RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_get_static(DateTime::class, "__construct");
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_get_static("phpversion");
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_get_static(Foo::class, "nostatics");
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}

try {
	uopz_get_static("nostatics");
} catch(RuntimeException $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECTF--
array(2) {
  ["vars"]=>
  array(5) {
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
  }
  ["bar"]=>
  NULL
}
array(2) {
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
  ["bar"]=>
  string(3) "bar"
}
string(%d) "failed to get statics from method %s::%s, it does not exist"
string(%d) "failed to get statics from function %s, it does not exist"
string(%d) "failed to get statics from internal method %s::%s"
string(%d) "failed to get statics from internal function %s"
string(%d) "failed to set statics in method %s::%s, no statics declared"
string(%d) "failed to set statics in function %s, no statics declared"
