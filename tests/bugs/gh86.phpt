--TEST--
set return on interface method
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php

interface EventEmitter {
    public function emitEvent($data);
}


trait Foo {
    public function emitEvent($data) {
        return $data+1;
    }
}

class Bar implements EventEmitter {
    use Foo;
}

$hook = function() {
    var_dump(__LINE__);
};

var_dump(uopz_set_return(EventEmitter::class, 'emitEvent', $hook, true));

$bar = new Bar;
$bar->emitEvent(1);
?>
--EXPECTF--
bool(true)
int(%d)
