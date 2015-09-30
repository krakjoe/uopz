/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2015                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Joe Watkins <krakjoe@php.net>                                |
  +----------------------------------------------------------------------+
 */
#ifndef HAVE_UOPZ_DISASSEMBLE
#define HAVE_UOPZ_DISASSEMBLE
/* {{{ */
static inline void uopz_disassemble_internal_function(zend_internal_function *function, zval *disassembly) {
	add_assoc_long(disassembly, "type", ZEND_INTERNAL_FUNCTION);
	add_assoc_long(disassembly, "flags", function->fn_flags);
	add_assoc_str(disassembly,   "name", zend_string_copy(function->function_name));
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_arginfo(zend_arg_info *arginfo, uint32_t end, zend_bool return_type, zval *disassembly) {
	zval result;
	uint32_t it = 0;

	array_init(&result);
	while (it < end) {
		zval arg;

		array_init(&arg);
		add_assoc_str(&arg, "name", zend_string_copy(arginfo[it].name));
		if (arginfo[it].class_name) {
			add_assoc_str(&arg, "class", zend_string_copy(arginfo[it].class_name));
		} else add_assoc_long(&arg, "type", arginfo[it].type_hint);
		add_assoc_bool(&arg, "reference", arginfo[it].pass_by_reference);
		add_assoc_bool(&arg, "null", arginfo[it].allow_null);
		add_assoc_bool(&arg, "variadic", arginfo[it].is_variadic);
		zend_hash_next_index_insert(Z_ARRVAL(result), &arg);
		it++;
	}

	zend_hash_str_add(Z_ARRVAL_P(disassembly), "arginfo", sizeof("arginfo"), &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_opcodes(zend_op *opcodes, uint32_t end, zval *disassembly) {
	uint32_t it = 0;
	zval result;

	array_init(&result);
	while (it < end) {
		zval opcode;
		zend_string *name = zend_hash_index_find_ptr(&UOPZ(opcodes), opcodes[it].opcode);

		array_init(&opcode);
		add_assoc_long(&opcode, "ext", opcodes[it].extended_value);
		add_assoc_str(&opcode, "opcode", zend_string_copy(name));	
		add_assoc_long(&opcode, "op1_type", opcodes[it].op1_type);
		add_assoc_long(&opcode, "op2_type", opcodes[it].op2_type);
		add_assoc_long(&opcode, "result_type", opcodes[it].result_type);

		zend_hash_next_index_insert(Z_ARRVAL(result), &opcode);
		it++;
	}
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "opcodes", sizeof("opcodes"), &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_vars(zend_string **vars, int end, zval *disassembly) {
	zval result;
	int it = 0;
	
	array_init(&result);
	while (it < end) {
		add_index_str(&result, it, zend_string_copy(vars[it]));
		it++;
	}
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "vars", sizeof("vars"), &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_literals(zval *literals, int end, zval *disassembly) {
	zval result;
	int it = 0;

	array_init(&result);
	while (it < end) {
		add_index_zval(&result, it, &literals[it]);
		it++;
	}
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "literals", sizeof("literals"), &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_function(zend_op_array *function, zval *disassembly) {
	zval literals;
	
	add_assoc_long(disassembly, "type", ZEND_USER_FUNCTION);
	add_assoc_long(disassembly, "flags", function->fn_flags);
	add_assoc_str(disassembly, "name",  zend_string_copy(function->function_name));
	if (function->scope) {
		add_assoc_str(disassembly, "scope", zend_string_copy(function->scope->name));
	}
	add_assoc_long(disassembly, "nargs", function->num_args);
	add_assoc_long(disassembly, "rnargs", function->required_num_args);
	
	if (function->arg_info) {
		uopz_disassemble_arginfo(function->arg_info, function->num_args, 0, disassembly);
	}

	uopz_disassemble_opcodes(function->opcodes, function->last, disassembly);
	uopz_disassemble_vars(function->vars, function->last_var, disassembly);
	uopz_disassemble_literals(function->literals, function->last_literal, disassembly);
} /* }}} */
#endif
