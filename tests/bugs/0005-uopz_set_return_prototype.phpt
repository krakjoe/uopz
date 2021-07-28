--TEST--
prototype mixup setting return
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Foo {
	public $bar;
	public function setBar ($value) { 
		$this->bar = $value;
	}
}

uopz_set_return (Foo::class, 'setBar', function ($value) {
	echo "unused\n";
}, true);

class Foo2 extends Foo {
	public function setBar ($value) { 
		$this->bar = 'expected';
	}
}

$foo2 = new Foo2;
$foo2->setBar ('meh');

var_dump($foo2->bar);
--EXPECT--
string(8) "expected"
