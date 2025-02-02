--TEST--
handle ZEND_VERIFY_NEVER_TYPE when uopz.exit disabled
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
uopz.exit=0
opcache.enable_cli=0
xdebug.enable=0
--FILE--
<?php

uopz_allow_exit(false);
function x(): never {
  exit(10);
}

x();

var_dump(uopz_get_exit_status());

uopz_allow_exit(true);

exit(20);

echo "not here\n";

?>
--EXPECT--
int(10)