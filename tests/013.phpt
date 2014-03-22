--TEST--
Test extend
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class My {
	
}

trait IMy {
	public function is() {
		
	}
}

uopz_extend(My::class, IMy::class);

$my = new My();

var_dump(method_exists($my, "is"));
?>
--EXPECT--
bool(true)


