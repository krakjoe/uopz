--TEST--
github #53
--DESCRIPTION--
uopz_redefine() refuses to redefine a constant as array
--SKIPIF--
<?php
include(__DIR__ . '/../skipif.inc');
?>
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
