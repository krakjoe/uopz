<?php
function show_disassembly_diff(array $one, array $two) {
	printf("disassembly failures:\n");

	if (@$one["arginfo"] != @$two["arginfo"]) {	
		printf("arginfo failure:\n");
		var_dump(@$one["arginfo"], @$two["arginfo"]);
		printf("\n");
	}

	if (@$one["opcodes"] != @$two["opcodes"]) {
		printf("opcodes failure:\n");
		foreach (@$one["opcodes"] as $idx => $op) {
			if (@$two["opcodes"][$idx] != $op) {
				printf("#%d:\n", $idx);
				var_dump($op, @$two["opcodes"][$idx]);
				printf("\n");
			}
		}
	}

	if (@$one["brk"] != @$two["brk"]) {
		printf("brk failure:\n");
		foreach (@$one["brk"] as $idx => $brk) {
			if (@$two["brk"][$idx] != $brk) {
				printf("#%d:\n", $idx);
				var_dump($brk, @$two["brk"][$idx]);
				printf("\n");
			}
		}
	}
	
	if (@$one["try"] != @$two["try"]) {
		printf("try failure:\n");
		foreach (@$one["try"] as $idx => $try) {
			if (@$two["try"][$idx] != $try) {
				printf("#%d:\n", $idx);
				var_dump($try, @$two["try"][$idx]);
				printf("\n");
			}
		}
	}

	if (@$one["scope"] != @$two["scope"]) {
		printf("scope failure: %s != %s\n", 
				@$one["scope"],
				@$two["scope"]);
	}

	var_dump($one, $two);
}

function run_disassembler_test(Closure $test, array $args) {
	$disassembled = uopz_disassemble($test);
	$assembled    = uopz_assemble($disassembled);
	$validation   = uopz_disassemble($assembled);

	if ($validation == $disassembled) {
		$expected = $test(...$args);
		$result   = $assembled(...$args);
		if ($expected != $result) {
			var_dump($expected, $result);
		} else var_dump($result);
	} else show_disassembly_diff($disassembled, $validation);
}

function run_disassembler_method_test(array $test, array $args) {
	$disassembled = uopz_disassemble(get_class($test[0]), $test[1]);
	$assembled    = uopz_assemble($disassembled, $test[0]);
	$validation   = uopz_disassemble($assembled);

	if ($validation == $disassembled) {
		$disassembled["flags"] = ["public"];
		$assembled = uopz_assemble($disassembled, $test[0]);
		$expected = $test(...$args);
		$result   = $assembled(...$args);
		if ($expected != $result) {
			var_dump($expected, $result);
		} else var_dump($result);
	} else show_disassembly_diff($disassembled, $validation);
}
?>
