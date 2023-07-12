--TEST--
handle ZEND_VERIFY_NEVER_TYPE when uopz.exit disabled
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
uopz.exit=0
opcache.enable_cli=0
xdebug.enable=0
--FILE--
<?php

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