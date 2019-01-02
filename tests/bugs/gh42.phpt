--TEST--
uopz_set_mock consistency
--SKIPIF--
<?php include("skipif.inc") ?>
--FILE--
<?php

class A {
  public function __construct() {
    echo "A ctor\n";
  }
  public function who() {
    echo "A\n";
  }
}
class B extends A {
  public function __construct() {
    echo "B ctor\n";
    parent::__construct();
  }
}
class MockA {
  public function __construct() {
    echo "MockA ctor\n";
  }
  public function who() {
    echo "MockA\n";
  }
}
uopz_set_mock(A::class, MockA::class);
$a = new A();
$a->who();
$b = new B();
$b->who();
?>
--EXPECT--
MockA ctor
MockA
B ctor
MockA ctor
MockA
