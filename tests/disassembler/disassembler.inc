<?php
function run_disassembler_test(Closure $test, array $args) {
	$disassembled = uopz_disassemble($test);
	$assembled    = uopz_assemble($disassembled);
	$validation   = uopz_disassemble($assembled);
	if ($validation == $disassembled) {
		var_dump($assembled(...$args));
	}
}
?>
