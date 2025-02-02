--TEST--
do not handle ZEND_VERIFY_NEVER_TYPE if not uopz_allow_exit
--EXTENSIONS--
uopz
--SKIPIF--
<?php
uopz_allow_exit(true);
if (version_compare(PHP_VERSION, '8.1.0', '<')) {
  die("skip PHP 8.1+ only");
}
--INI--
uopz.disable=0
uopz.exit=1
opcache.enable_cli=0
xdebug.enable=0
--FILE--
<?php

function x(): never {
  return "here";
}
x();

?>
--EXPECTF--
Fatal error: A never-returning function must not return in %s