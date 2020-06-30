--TEST--
call uopz_set_property in a class scope
--SKIPIF--
<?php include("../skipif.inc") ?>
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
    private $name = 'hello';

    public function getName() {
        return $this->name;
    }
}

class UM {
    public function setProp($obj, $name, $value) {
        uopz_set_property($obj, $name, $value);
    }
}

$f = new Foo();

$um = New UM();
$um->setProp($f, 'name', 'world');

echo $f->getName();
?>
--EXPECT--
world
