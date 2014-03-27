--TEST--
Test copy user functions
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class A
{
	private function foo ()
	{
		static $vars = [
		 	'one', 'two'
		];

		if (count($vars) == 2) {
			$vars[] = mt_rand();
			/* only output one set of vars if these are set, passing test */
			var_dump ($vars);
	    }
	}

	public function testOne ()
	{
		$foo = uopz_copy(A::class, 'foo');
		
		return $foo();
	}

	public function testTwo ()
	{
		$foo = uopz_copy(A::class, 'foo');
		
		return $foo();
	}
}

$a = new A ();
$a->testOne ();
$a->testTwo ();
--EXPECTF--
array(3) {
  [0]=>
  string(3) "one"
  [1]=>
  string(3) "two"
  [2]=>
  int(%d)
}
array(3) {
  [0]=>
  string(3) "one"
  [1]=>
  string(3) "two"
  [2]=>
  int(%d)
}

