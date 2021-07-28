--TEST--
uopz_extend affects only explicit calls via parent:: but not inherited methods
--EXTENSIONS--
uopz
--SKIPIF--
<?php
uopz_allow_exit(true);
if (version_compare(PHP_VERSION, '7.4', '>=')
	&& function_exists('opcache_get_status')
	&& ($status = opcache_get_status())
	&& $status['opcache_enabled'])
{
	die('skip not for PHP 7.4+ with OPcache');
}
?>
--INI--
uopz.disable=0
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

