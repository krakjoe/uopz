--TEST--
Test implement
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
class My {
	public function is() {}
}

interface IMy {
	public function is();
}

uopz_implement(My::class, IMy::class);

$my = new My();

var_dump($my instanceof IMy);
?>
--EXPECT--
bool(true)


