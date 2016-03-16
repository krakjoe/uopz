--TEST--
Test backup/restore
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
/*
 Test backup/restore internal function
*/
var_dump(uopz_backup("strlen"));
var_dump(uopz_restore("strlen"));

/*
 Test backup/restore user function
*/

class Test {
	public static function method() {
		return false;
	}
}

var_dump(uopz_backup(Test::class, "method"));
var_dump(Test::method());
uopz_function(Test::class, "method", function(){
	return true;
});
var_dump(Test::method());
var_dump(uopz_restore(Test::class, "method"));
var_dump(Test::method());
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(false)
bool(true)
bool(true)
bool(false)
