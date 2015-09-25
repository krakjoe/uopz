--TEST--
Test backup/restore/rename combination
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function original() { 
	static $vars = [
		'one', 'two'
	]; 	
	
	if (count($vars) == 2) {
		$vars[] = mt_rand();
	}
	
	var_dump($vars);
}

var_dump(uopz_backup("original"));
uopz_rename("original", "copied");

original();
copied();

var_dump(uopz_restore("original"));
?>
--EXPECTF--
bool(true)
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
bool(true)
