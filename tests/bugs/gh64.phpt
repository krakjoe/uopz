--TEST--
Segfault after uopz_set_static
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class A {
    function fn() {
        static $a = 1;
        $a++;
    }
}

print_r(uopz_get_static(A::class, "fn"));
uopz_set_static(A::class, "fn", ['a'=>3, 'b'=>7]);
$a = new A;
$a->fn();
print_r(uopz_get_static(A::class, "fn"));
--EXPECT--
Array
(
    [a] => 1
)
Array
(
    [a] => 4
)

