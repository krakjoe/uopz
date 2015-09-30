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

#define UOPZ_HAS_RETURN_TYPE(f) (((f)->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) == ZEND_ACC_HAS_RETURN_TYPE)
#define UOPZ_ZVAL_NUM(c) (c > 0L ? (c) / sizeof(zval) : c)
#define UOPZ_JMP_NUM(n) ((n / (sizeof(znode_op) * sizeof(znode_op))) - 1)
#define UOPZ_CV_NUM(c) ((c - sizeof(zend_execute_data)) / sizeof(zval))
#define UOPZ_VAR_NUM(c) (c > 0L ? c / sizeof(zend_string) : c)

/* {{{ */
static inline zend_string* uopz_type_name(zend_uchar type) {
	return zend_string_copy(UOPZ(types)[type]);	
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_arginfo(zend_arg_info *arginfo, uint32_t end, zend_bool return_type, zval *disassembly) {
	zval result;
	uint32_t it = 0;

	array_init(&result);
	if (return_type) {
		zval ret;
		zend_arg_info *return_type = (arginfo - 1);

		array_init(&ret);
		if (return_type->class_name) {
			add_assoc_str(&ret, "class", zend_string_copy(return_type->class_name));
		} else if (return_type->type_hint != IS_UNDEF) {
			add_assoc_str(&ret, "type", uopz_type_name(arginfo[it].type_hint));
		}

		zend_hash_index_add(Z_ARRVAL(result), -1, &ret);		
	}

	while (it < end) {
		zval arg;

		array_init(&arg);
		add_assoc_str(&arg, "name", zend_string_copy(arginfo[it].name));
		if (arginfo[it].class_name) {
			add_assoc_str(&arg, "class", zend_string_copy(arginfo[it].class_name));
		} else if (arginfo[it].type_hint != IS_UNDEF) {
			add_assoc_str(&arg, "type", uopz_type_name(arginfo[it].type_hint));
		}
		add_assoc_bool(&arg, "reference", arginfo[it].pass_by_reference);
		add_assoc_bool(&arg, "null", arginfo[it].allow_null);
		add_assoc_bool(&arg, "variadic", arginfo[it].is_variadic);

		zend_hash_next_index_insert(Z_ARRVAL(result), &arg);
		it++;
	}

	zend_hash_str_add(Z_ARRVAL_P(disassembly), "arginfo", sizeof("arginfo") - 1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_internal_arginfo(zend_internal_arg_info *arginfo, uint32_t end, zend_bool return_type, zval *disassembly) {
	zval result;
	uint32_t it = 0;

	array_init(&result);
	if (return_type) {
		zval ret;
		zend_internal_arg_info *return_type = (arginfo - 1);

		array_init(&ret);
		if (return_type->class_name) {
			zend_string *class_name = zend_string_init(
				return_type->class_name, strlen(return_type->class_name), 0);

			add_assoc_str(&ret, "class", class_name);
		} else if (return_type->type_hint != IS_UNDEF) {
			add_assoc_str(&ret, "type", uopz_type_name(arginfo[it].type_hint));
		}

		zend_hash_index_add(Z_ARRVAL(result), -1, &ret);		
	}

	while (it < end) {
		zval arg;
		zend_string *name = zend_string_init(
			arginfo[it].name, strlen(arginfo[it].name), 0);

		array_init(&arg);
		add_assoc_str(&arg, "name", name);
		if (arginfo[it].class_name) {
			zend_string *class_name = zend_string_init(
				arginfo[it].class_name, strlen(arginfo[it].class_name), 0);

			add_assoc_str(&arg, "class", class_name);
		} else if (arginfo[it].type_hint != IS_UNDEF) {
			add_assoc_str(&arg, "type", uopz_type_name(arginfo[it].type_hint));
		}
		add_assoc_bool(&arg, "reference", arginfo[it].pass_by_reference);
		add_assoc_bool(&arg, "null", arginfo[it].allow_null);
		add_assoc_bool(&arg, "variadic", arginfo[it].is_variadic);

		zend_hash_next_index_insert(Z_ARRVAL(result), &arg);
		it++;
	}

	zend_hash_str_add(Z_ARRVAL_P(disassembly), "arginfo", sizeof("arginfo") - 1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_operand(char *name, size_t nlen, zend_op *opcodes, zend_op *opline, zend_uchar op_type, znode_op *op, zend_bool jmp, zval *disassembly) {
	zval result;

	if (op_type == IS_UNUSED && !jmp)
		return;

	array_init(&result);

	if (jmp) {
		add_assoc_long(&result, "jmp", OP_JMP_ADDR(opline, *op) - opcodes);
	} else switch (op_type) {
		case IS_TMP_VAR:
			add_assoc_long(&result, "tmp", UOPZ_ZVAL_NUM(op->var));		
		break;

		case IS_CV:
			add_assoc_long(&result, "cv", UOPZ_CV_NUM(op->num));
		break;

		case IS_CONST:
			add_assoc_long(&result, "constant", UOPZ_ZVAL_NUM(op->constant));
		break;

		case IS_VAR:
			add_assoc_long(&result, "var",      UOPZ_VAR_NUM(op->var));
		break;
	}
	
	zend_hash_str_add(Z_ARRVAL_P(disassembly), name, nlen, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_opcodes(zend_op *opcodes, uint32_t end, zend_string **vars, zval *literals, zval *disassembly) {
	uint32_t it = 0;
	zval result;

	array_init(&result);
	while (it < end) {
		zval opcode;
		zend_string *name = zend_hash_index_find_ptr(&UOPZ(opcodes), opcodes[it].opcode);

		array_init(&opcode);
		if (name) {
			add_assoc_str(&opcode, "opcode", zend_string_copy(name));
		} else add_assoc_long(&opcode, "opcode", opcodes[it].opcode);

		switch (opcodes[it].opcode) {
			case ZEND_JMP:
			case ZEND_FAST_CALL:
			case ZEND_DECLARE_ANON_CLASS:
			case ZEND_DECLARE_ANON_INHERITED_CLASS:
				uopz_disassemble_operand(ZEND_STRL("op1"), opcodes, &opcodes[it], opcodes[it].op1_type, &opcodes[it].op1, 1, &opcode);
				uopz_disassemble_operand(ZEND_STRL("op2"), opcodes, &opcodes[it], opcodes[it].op2_type, &opcodes[it].op2, 0, &opcode);
			break;

			case ZEND_JMPZNZ:
			case ZEND_JMPZ:
			case ZEND_JMPNZ:
			case ZEND_JMPZ_EX:
			case ZEND_JMPNZ_EX:
			case ZEND_JMP_SET:
			case ZEND_COALESCE:
			case ZEND_NEW:
			case ZEND_FE_RESET_R:
			case ZEND_FE_RESET_RW:
			case ZEND_ASSERT_CHECK:
				uopz_disassemble_operand(ZEND_STRL("op1"), opcodes, &opcodes[it], opcodes[it].op1_type, &opcodes[it].op1, 0, &opcode);
				uopz_disassemble_operand(ZEND_STRL("op2"), opcodes, &opcodes[it], opcodes[it].op2_type, &opcodes[it].op2, 1, &opcode);
			break;

			default:
				uopz_disassemble_operand(ZEND_STRL("op1"), opcodes, &opcodes[it], opcodes[it].op1_type, &opcodes[it].op1, 0, &opcode);
				uopz_disassemble_operand(ZEND_STRL("op2"), opcodes, &opcodes[it], opcodes[it].op2_type, &opcodes[it].op2, 0, &opcode);
		}
		
		uopz_disassemble_operand(ZEND_STRL("result"), opcodes, &opcodes[it], opcodes[it].result_type, &opcodes[it].result, 0, &opcode);
		if (opcodes[it].extended_value > 0L) {
			switch (opcodes[it].opcode) {
				/* horrible, don't keep allocing these */
				case ZEND_FETCH_UNSET:
				case ZEND_FETCH_RW:
				case ZEND_FETCH_W:
				case ZEND_FETCH_R: switch (opcodes[it].extended_value & ZEND_FETCH_TYPE_MASK) {
					case ZEND_FETCH_GLOBAL_LOCK:
					case ZEND_FETCH_GLOBAL:
						add_assoc_str(&opcode, "fetch", zend_string_init(ZEND_STRL("global"), 0));
					break;

					case ZEND_FETCH_STATIC:
						add_assoc_str(&opcode, "fetch", zend_string_init(ZEND_STRL("static"), 0));
					break;

					case ZEND_FETCH_LOCAL:
						add_assoc_str(&opcode, "fetch", zend_string_init(ZEND_STRL("local"), 0));
					break;
				} break;
			}
		}
			

		zend_hash_next_index_insert(Z_ARRVAL(result), &opcode);
		it++;
	}
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "opcodes", sizeof("opcodes") - 1, &result);
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
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "vars", sizeof("vars") - 1, &result);
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
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "literals", sizeof("literals") - 1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_statics(HashTable *statics, zval *disassembly) {
	if (statics) {
		zval tmp;
		ZVAL_ARR(&tmp, statics);
		add_assoc_zval(disassembly, "static", &tmp);
		Z_ADDREF(tmp);
	}
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_flags(zend_uchar flags, zval *disassembly) {
	zval result;

	array_init(&result);
	
	if (flags & ZEND_ACC_STATIC) {
		add_assoc_bool(&result, "static", 1);
	}

	if (flags & ZEND_ACC_PROTECTED) {
		add_assoc_bool(&result, "protected", 1);
	} else if (flags & ZEND_ACC_PRIVATE) {
		add_assoc_bool(&result, "private", 1);
	} else add_assoc_bool(&result, "public", 1);

	if (flags & ZEND_ACC_ABSTRACT) {
		add_assoc_bool(&result, "abstract", 1);
	}
	
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "flags", sizeof("flags") - 1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_internal_function(zend_internal_function *function, zval *disassembly) {
	add_assoc_str(disassembly,   "name", zend_string_copy(function->function_name));
	uopz_disassemble_flags(function->fn_flags, disassembly);
	if (function->scope) {
		add_assoc_str(disassembly, "scope", zend_string_copy(function->scope->name));
	}
	add_assoc_long(disassembly, "nargs", function->num_args);
	add_assoc_long(disassembly, "rnargs", function->required_num_args);
	
	if (function->arg_info)
		uopz_disassemble_internal_internal_arginfo(function->arg_info, function->num_args, UOPZ_HAS_RETURN_TYPE(function), disassembly);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_function(zend_op_array *function, zval *disassembly) {

	add_assoc_str(disassembly, "name",  zend_string_copy(function->function_name));	

	uopz_disassemble_flags(function->fn_flags, disassembly);
	if (function->scope) {
		add_assoc_str(disassembly, "scope", zend_string_copy(function->scope->name));
	}
	add_assoc_long(disassembly, "nargs", function->num_args);
	add_assoc_long(disassembly, "rnargs", function->required_num_args);
	
	if (function->arg_info)
		uopz_disassemble_arginfo(function->arg_info, function->num_args, UOPZ_HAS_RETURN_TYPE(function), disassembly);

	uopz_disassemble_opcodes(function->opcodes, function->last, function->vars, function->literals, disassembly);
	uopz_disassemble_vars(function->vars, function->last_var, disassembly);
	uopz_disassemble_literals(function->literals, function->last_literal, disassembly);
	uopz_disassemble_statics(function->static_variables, disassembly);
} /* }}} */
#endif
