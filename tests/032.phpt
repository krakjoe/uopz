--TEST--
init ns fcall
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
opcache.enable_cli=0
--FILE--
<?php
namespace Moo {
	function fail() {
		return none();
	}

	function say() {
		return call();
	}

	function call() {
		return true;
	}
}

namespace {
	var_dump(Moo\say());

	Moo\fail();
}
--EXPECTF--
bool(true)

Fatal error: Uncaught Error: Call to undefined function Moo\none() in %s:4
Stack trace:
#0 %s(19): Moo\fail()
#1 {main}
  thrown in %s on line 4
