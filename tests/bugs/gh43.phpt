--TEST--
github #43
--DESCRIPTION--
Setting hook on __invoke method doesn't work on call_user_func
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
class Handler {
    public function __invoke($arg)
    {
        return $arg;
    }
}

uopz_set_hook(Handler::class, '__invoke', function (...$args)
{
    echo "hi!";
});

$h = new Handler();
var_dump($h(1));
var_dump($h->__invoke(2));
var_dump(call_user_func($h, 3));

uopz_set_return(Handler::class, '__invoke', 42);

var_dump($h(1));
var_dump($h->__invoke(2));
var_dump(call_user_func($h, 3));
--EXPECT--
hi!int(1)
hi!int(2)
hi!int(3)
hi!int(42)
hi!int(42)
hi!int(42)
