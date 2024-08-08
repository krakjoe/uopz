--TEST--
github #53
--DESCRIPTION--
uopz_redefine() refuses to redefine a constant as array
--SKIPIF--
<?php
uopz_allow_exit(true);
$protect = extension_loaded("Zend OPcache")
	&& ($conf = opcache_get_configuration()["directives"])
	&& array_key_exists("opcache.protect_memory", $conf)
	&&  $conf["opcache.protect_memory"];
if ($protect) die("xfail known issues with constant redefinition; see #151");
?>
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class testClass
{
    const ARR = [1, 2, 3];
}

uopz_redefine(testClass::class, 'ARR', [1, 2, 4]);
var_dump(testClass::ARR);
?>
===DONE===
--EXPECT--
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(4)
}
===DONE===
