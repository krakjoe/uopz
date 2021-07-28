--TEST--
get hook not case insensitive
--EXTENSIONS--
uopz
--INI--
uopz.disable=0
--FILE--
<?php
declare(strict_types=1);

namespace name {

    const TEST = 1;
}

namespace {

    echo NAME\TEST;

    uopz_redefine('name\\TEST', 2);

    echo name\TEST;

    uopz_redefine('naMe\\TEST', 3);

    echo NAME\TEST;

    uopz_redefine('NAME\\TEST', 4);
    
    echo NAME\TEST;
}
?>
--EXPECT--
1234
