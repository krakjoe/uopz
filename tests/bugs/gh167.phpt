--TEST--
uopz_set_return() overload confusion
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {}
function foo(): int {
    return 42;
}
uopz_set_return("foo", function() {return 4711;}, true);
var_dump(foo());
?>
--EXPECT--
int(4711)
