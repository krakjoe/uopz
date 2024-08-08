--TEST--
opcache constant substitution disable
--EXTENSIONS--
uopz
--SKIPIF--
<?php
	uopz_allow_exit(true);
	$opcache = ini_get("opcache.enable_cli");
	if ($opcache === false || $opcache === "0") die("skip opcache required");

    $protect = extension_loaded("Zend OPcache")
        && ($conf = opcache_get_configuration()["directives"])
        && array_key_exists("opcache.protect_memory", $conf)
        &&  $conf["opcache.protect_memory"];
    if ($protect) die("xfail known issues with constant redefinition; see #151");
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

