--TEST--
Test rename
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
function _My_strlen($string) {
	return strlen($string) * 5;
}

class MyClass {
	public function First() {
		
	}
}

uopz_rename("_my_strlen", "my_other_strlen");
uopz_rename("myClass", "first", "second");

var_dump(my_other_strlen("hello world"));

$my = new myClass();
var_dump(method_exists($my, "second"));

class My {
	public function search() {
		var_dump(__METHOD__);	
	}
	
	public function replace() {
		var_dump(__METHOD__);
	}
}

if (!function_exists("session_start")) {
	function session_start() {
		var_dump(__FUNCTION__);
	}
}

function session_start_original() {
	var_dump(__FUNCTION__);
}

uopz_rename("session_start", "session_start_original");
uopz_rename("My", "search", "replace");

$my = new My();
$my->search();

var_dump(function_exists("session_start"),
		 function_exists("session_start_original"));

session_start();
?>
--EXPECTF--
int(55)
bool(true)
string(11) "My::replace"
bool(true)
bool(true)
string(22) "session_start_original"
