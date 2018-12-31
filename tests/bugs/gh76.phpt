--TEST--
uopz_extend affects only explicit calls via parent:: but not inherited methods
--SKIPIF--

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
uopz_extend(B::class, MockA::class);
$b = new B();
$b->who();
?>
--EXPECT--
B ctor
MockA ctor
MockA

