--TEST--
opcache constant substitution disable
--SKIPIF--
<?php
	include("skipif.inc");
	uopz_allow_exit(true);
	if (!ini_get("opcache.enable_cli")) die("skip opcache required");
?>
--INI--
uopz.disable=0
opcache.enabled=1
opcache.enable_cli=1
opcache.optimization_level=0x7FFFBFFF
--FILE--
<?php
class Foo {
    const USE_B = false;
 
    public function __call ($method, $arguments) {
        return self::USE_B ? $this->callB () : $this->callA ();
    }
 
    private function callA () {
        echo "A!" . PHP_EOL;
    }
    private function callB () {
        echo "B!" . PHP_EOL;
    }
}
 
uopz_set_return (Foo::class, 'callA', function () { echo "uopz A!" . PHP_EOL; }, true);
 
$x = new Foo;
var_dump ($x->__call (null, null));
 
uopz_redefine (Foo::class, 'USE_B', true);
var_dump ($x->__call (null, null));
?>
--EXPECT--
uopz A!
NULL
B!
NULL

