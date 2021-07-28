--TEST--
hook closure call inconsistency
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php

declare(strict_types=1);

class Test {
    public function some()
    {

    }
}

uopz_set_hook(Test::class, 'some', function (...$args) {
    var_dump($args);
});

call_user_func([new Test(), 'some'], 1, 2);
call_user_func_array([new Test(), 'some'], [1, 2]);
(new Test())->some(1, 2);
?>
--EXPECT--
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}


