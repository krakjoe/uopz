--TEST--
Test auto restore user methods
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class foo {
	public static function bar () { return true; }
}

uopz_backup('foo', 'bar');
uopz_function('foo', 'bar', function(){
	return false;
}, true);
var_dump(foo::bar());
?>
--EXPECT--
bool(false)

