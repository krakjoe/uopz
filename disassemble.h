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
#define UOPZ_ZVAL_NUM(c) 		((c > 0L ? (c) / sizeof(zval) : c) - ZEND_CALL_FRAME_SLOT)
#define UOPZ_CONST_NUM(c) 		((c > 0L ? (c) / sizeof(zval) : c))
#define UOPZ_CV_NUM(c) 			((c - sizeof(zend_execute_data)) / sizeof(zval))
#define UOPZ_VAR_NUM(c) 		(c > 0L ? c / sizeof(zend_string) : c)

/* {{{ */
static inline void uopz_disassembler_destroy_opcodes(zval *zv) {
	zend_string_release(Z_PTR_P(zv));
}

static inline void uopz_disassembler_init_opcodes(HashTable *hash) {
	uint32_t it = 1, 
			  end = 159;
	
	zend_hash_init(hash, end, NULL, uopz_disassembler_destroy_opcodes, 0);
	while (it < end) {
		zend_string  *name;
		const char *opcode = zend_get_opcode_name(it);

		if (!opcode) {
			it++;
			continue;
		}

		name  = zend_string_init(
			opcode, strlen(opcode), 0);
		zend_hash_index_add_ptr(hash, it, name);
		it++;
	}
} /* }}} */

/* {{{ */
static inline void uopz_disassembler_init_types(zend_string  **types) {
	memset(types, 0, sizeof(zend_string*) * UOPZ_NUM_TYPES);

	/* some of these will be unused */
	types[IS_UNDEF] = zend_string_init(ZEND_STRL("undefined"), 0);
	types[IS_NULL]	= zend_string_init(ZEND_STRL("null"), 0);
	types[IS_FALSE]	= zend_string_init(ZEND_STRL("false"), 0);
	types[IS_TRUE]	= zend_string_init(ZEND_STRL("true"), 0);
	types[IS_LONG]	= zend_string_init(ZEND_STRL("int"), 0);
	types[IS_DOUBLE]	= zend_string_init(ZEND_STRL("double"), 0);
	types[IS_STRING]	= zend_string_init(ZEND_STRL("string"), 0);
	types[IS_ARRAY]		= zend_string_init(ZEND_STRL("array"), 0);
	types[IS_OBJECT]	= zend_string_init(ZEND_STRL("object"), 0);
	types[IS_RESOURCE]	= zend_string_init(ZEND_STRL("resource"), 0);
	types[IS_REFERENCE] = zend_string_init(ZEND_STRL("reference"), 0);
	types[IS_CONSTANT]	= zend_string_init(ZEND_STRL("constant"), 0);
	types[IS_CONSTANT_AST]	= zend_string_init(ZEND_STRL("ast"), 0);
	types[_IS_BOOL]			= zend_string_init(ZEND_STRL("bool"), 0);
	types[IS_CALLABLE]		= zend_string_init(ZEND_STRL("callable"), 0);
	types[IS_INDIRECT]		= zend_string_init(ZEND_STRL("indirect"), 0);
	types[IS_PTR]			= zend_string_init(ZEND_STRL("pointer"), 0);
} /* }}} */

/* {{{ */
static inline void uopz_disassembler_init_modifiers(HashTable *hash) {
	zval tmp;

	zend_hash_init(hash, 10, NULL, ZVAL_PTR_DTOR, 0);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("unknown"), 0));
	zend_hash_index_add(hash, -1, &tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("public"), 0));
	zend_hash_index_add(hash, ZEND_ACC_PUBLIC, &tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("protected"), 0));
	zend_hash_index_add(hash, ZEND_ACC_PROTECTED, &tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("private"), 0));
	zend_hash_index_add(hash, ZEND_ACC_PRIVATE, &tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("final"), 0));
	zend_hash_index_add(hash, ZEND_ACC_FINAL, &tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("static"), 0));
	zend_hash_index_add(hash, ZEND_ACC_STATIC, &tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("reference"), 0));
	zend_hash_index_add(hash, ZEND_ACC_RETURN_REFERENCE, &tmp);
} /* }}} */

/* {{{ */
static inline zend_string *uopz_disassemble_modifier_name(uint32_t modifier) {
	zval *name = zend_hash_index_find(&UOPZ(modifiers), modifier);

	if (name) {
		return zend_string_copy(Z_STR_P(name));
	} else return uopz_disassemble_modifier_name(-1);
}
/* }}} */

/* {{{ */
static inline void uopz_disassembler_init_fetches(HashTable *hash) {
	zval tmp;

	zend_hash_init(hash, 4, NULL, ZVAL_PTR_DTOR, 0);	
	
	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("global"), 0));
	zend_hash_index_add(hash, ZEND_FETCH_GLOBAL, &tmp);
	zend_hash_index_add(hash, ZEND_FETCH_GLOBAL_LOCK, &tmp);
	Z_ADDREF(tmp);

	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("static"), 0));
	zend_hash_index_add(hash, ZEND_FETCH_STATIC, &tmp);
	
	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL("local"), 0));
	zend_hash_index_add(hash, ZEND_FETCH_LOCAL, &tmp);
} /* }}} */

/* {{{ */
static inline void uopz_disassembler_init() {
	uopz_disassembler_init_opcodes(&UOPZ(opcodes));
	uopz_disassembler_init_types(UOPZ(types));
	uopz_disassembler_init_fetches(&UOPZ(fetches));
	uopz_disassembler_init_modifiers(&UOPZ(modifiers));
} /* }}} */

/* {{{ */
static inline void uopz_disassembler_shutdown() {
	zend_hash_destroy(&UOPZ(opcodes));
	zend_hash_destroy(&UOPZ(fetches));
	zend_hash_destroy(&UOPZ(modifiers));
	{
		uint32_t it = IS_UNDEF, end = IS_PTR;
		
		while (it <= end) {
			if (UOPZ(types)[it])
				zend_string_release(UOPZ(types)[it]);
			it++;
		}
	}
	
} /* }}} */

/* {{{ */
static inline zend_string* uopz_disassemble_type_name(zend_uchar type) {
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
			add_assoc_str(&ret, "type", uopz_disassemble_type_name(arginfo[it].type_hint));
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
			add_assoc_str(&arg, "type", uopz_disassemble_type_name(arginfo[it].type_hint));
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
			add_assoc_str(&ret, "type", uopz_disassemble_type_name(arginfo[it].type_hint));
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
			add_assoc_str(&arg, "type", uopz_disassemble_type_name(arginfo[it].type_hint));
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
			add_assoc_long(&result, "constant", UOPZ_CONST_NUM(op->constant));
		break;

		case IS_VAR:
			add_assoc_long(&result, "var",      UOPZ_VAR_NUM(op->var));
		break;
	}
	
	zend_hash_str_add(Z_ARRVAL_P(disassembly), name, nlen, &result);
} /* }}} */

/* {{{ */
static inline void upoz_disassemble_extended_value(zend_uchar opcode, uint32_t extended_value, zval *disassembly) {
	switch (opcode) {
		case ZEND_FETCH_UNSET:
		case ZEND_FETCH_RW:
		case ZEND_FETCH_W:
		case ZEND_FETCH_R: {
			zval *type = zend_hash_index_find(&UOPZ(fetches), extended_value & ZEND_FETCH_TYPE_MASK);

			switch (extended_value & ZEND_FETCH_TYPE_MASK) {
				case ZEND_FETCH_GLOBAL_LOCK:
				case ZEND_FETCH_GLOBAL:
					add_assoc_zval(disassembly, "fetch", type);
					Z_ADDREF_P(type);
				break;

				case ZEND_FETCH_STATIC:
					add_assoc_zval(disassembly, "fetch", type);
					Z_ADDREF_P(type);
				break;

				case ZEND_FETCH_LOCAL:
					add_assoc_zval(disassembly, "fetch", type);
					Z_ADDREF_P(type);
				break;
			}
		} break;

		case ZEND_CAST:
			add_assoc_str(disassembly, "type", uopz_disassemble_type_name(extended_value));
		break;

		/* TODO(krakjoe) these ... */
	}
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
			upoz_disassemble_extended_value(opcodes[it].opcode, opcodes[it].extended_value, &opcode);
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
static inline void uopz_disassemble_brk(zend_brk_cont_element *brk, int end, zval *disassembly) {
	zval result;
	int it = 0;

	if (!end)
		return;

	array_init(&result);
	while (it < end) {
		zval tmp;

		array_init(&tmp);
		if (brk[it].start > -1)
			add_assoc_long(&tmp, "start", 	brk[it].start);
		if (brk[it].cont > -1)
			add_assoc_long(&tmp, "cont",  	brk[it].cont);
		if (brk[it].brk > -1)
			add_assoc_long(&tmp, "brk",  	brk[it].brk);
		if (brk[it].parent > -1)
			add_assoc_long(&tmp, "parent",  brk[it].parent);
		add_index_zval(&result, it, &tmp);
		it++;
	}
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "brk", sizeof("brk")-1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_try(zend_try_catch_element *el, int end, zval *disassembly) {
	zval result;
	int it = 0;

	if (!end)
		return;	

	array_init(&result);	
	while (it < end) {
		zval tmp;
		array_init(&tmp);
		
		add_assoc_long(&tmp, "try", 	el[it].try_op);
		add_assoc_long(&tmp, "catch",  	el[it].catch_op);
		if (el[it].finally_op)
			add_assoc_long(&tmp, "finally", el[it].finally_op);
		if (el[it].finally_end)
			add_assoc_long(&tmp, "end",  	el[it].finally_end);
		add_index_zval(&result, it, &tmp);
		it++;
	}
	zend_hash_str_add(Z_ARRVAL_P(disassembly), "try", sizeof("try")-1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_flags(uint32_t flags, zval *disassembly) {
	zval result;

	array_init(&result);

	if (flags & ZEND_ACC_RETURN_REFERENCE)
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_RETURN_REFERENCE));

	if (flags & ZEND_ACC_ABSTRACT)
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_ABSTRACT));

	if (flags & ZEND_ACC_FINAL)
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_FINAL));

	if (flags & ZEND_ACC_STATIC)
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_STATIC));

	if (flags & ZEND_ACC_PROTECTED) {
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_PROTECTED));
	} else if (flags & ZEND_ACC_PRIVATE) {
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_PRIVATE));
	} else add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_PUBLIC));

	zend_hash_str_add(Z_ARRVAL_P(disassembly), "flags", sizeof("flags") - 1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_internal_function(zend_internal_function *function, zval *disassembly) {
	if (function->scope)
		add_assoc_str(disassembly, "scope", zend_string_copy(function->scope->name));
	add_assoc_str(disassembly,   "name", zend_string_copy(function->function_name));
	uopz_disassemble_flags(function->fn_flags, disassembly);
	add_assoc_long(disassembly, "nargs", function->num_args);
	add_assoc_long(disassembly, "rnargs", function->required_num_args);
	if (function->arg_info)
		uopz_disassemble_internal_internal_arginfo(function->arg_info, function->num_args, UOPZ_HAS_RETURN_TYPE(function), disassembly);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_function(zend_op_array *function, zval *disassembly) {
	if (function->scope)
		add_assoc_str(disassembly, "scope", zend_string_copy(function->scope->name));
	add_assoc_str(disassembly, "name",  zend_string_copy(function->function_name));
	uopz_disassemble_flags(function->fn_flags, disassembly);
	if (function->scope && function->this_var)
		add_assoc_long(disassembly, "this", UOPZ_CV_NUM(function->this_var));
	add_assoc_long(disassembly, "nargs", function->num_args);
	add_assoc_long(disassembly, "rnargs", function->required_num_args);
	if (function->arg_info)
		uopz_disassemble_arginfo(function->arg_info, function->num_args, UOPZ_HAS_RETURN_TYPE(function), disassembly);
	uopz_disassemble_opcodes(function->opcodes, function->last, function->vars, function->literals, disassembly);
	uopz_disassemble_vars(function->vars, function->last_var, disassembly);
	uopz_disassemble_literals(function->literals, function->last_literal, disassembly);
	uopz_disassemble_statics(function->static_variables, disassembly);
	uopz_disassemble_brk(function->brk_cont_array, function->last_brk_cont, disassembly);
	uopz_disassemble_try(function->try_catch_array, function->last_try_catch, disassembly);
} /* }}} */
#endif
