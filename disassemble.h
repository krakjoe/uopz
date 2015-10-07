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
	types[IS_DOUBLE]	= zend_string_init(ZEND_STRL("float"), 0);
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
static inline void uopz_disassembler_init_optypes(HashTable *types) {
#define REGISTER_OPTYPE(n, c) do { \
	zval tmp; \
	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL(n), 0)); \
	zend_hash_index_update(types, c, &tmp); \
} while(0)

	zend_hash_init(
		types, 5, NULL, ZVAL_PTR_DTOR, 1);
	
	REGISTER_OPTYPE("var",				IS_VAR);
	REGISTER_OPTYPE("tmp",				IS_TMP_VAR);
	REGISTER_OPTYPE("cv",				IS_CV);
	REGISTER_OPTYPE("constant",			IS_CONST);
	REGISTER_OPTYPE("unused",			IS_UNUSED);
	REGISTER_OPTYPE("ext-type-unused",	EXT_TYPE_UNUSED);
	REGISTER_OPTYPE("unknown",			-1);

#undef REGISTER_OPTYPE
} /* }}} */

/* {{{ */
static inline zend_string *uopz_disassemble_optype(zend_uchar type) {
	zval *disassembly = zend_hash_index_find(&UOPZ(optypes), type &~ EXT_TYPE_UNUSED);
	
	if (type && Z_TYPE_P(disassembly) == IS_STRING) {
		return zend_string_copy(Z_STR_P(disassembly));
	}

	return uopz_disassemble_optype(-1); 
} /* }}} */

/* {{{ */
static inline void uopz_disassembler_init_modifiers(HashTable *hash) {
	zval tmp;

#define REGISTER_MODIFIER(n, m) do { \
	zval tmp; \
	ZVAL_STR(&tmp, zend_string_init(ZEND_STRL(n), 0)); \
	zend_hash_index_add(hash, m, &tmp); \
} while(0)

	zend_hash_init(hash, 11, NULL, ZVAL_PTR_DTOR, 0);

	REGISTER_MODIFIER("unknown",	-1);
	REGISTER_MODIFIER("public",		ZEND_ACC_PUBLIC);
	REGISTER_MODIFIER("protected",	ZEND_ACC_PROTECTED);
	REGISTER_MODIFIER("private",	ZEND_ACC_PRIVATE);
	REGISTER_MODIFIER("final",		ZEND_ACC_FINAL);
	REGISTER_MODIFIER("static", 	ZEND_ACC_STATIC);
	REGISTER_MODIFIER("reference", 	ZEND_ACC_RETURN_REFERENCE);
	REGISTER_MODIFIER("generator", 	ZEND_ACC_GENERATOR);

#undef REGISTER_MODIFIER
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
static inline void uopz_disassembler_init() {
	uopz_disassembler_init_opcodes(&UOPZ(opcodes));
	uopz_disassembler_init_modifiers(&UOPZ(modifiers));
	uopz_disassembler_init_optypes(&UOPZ(optypes));
	uopz_disassembler_init_types(UOPZ(types));	
} /* }}} */

/* {{{ */
static inline void uopz_disassembler_shutdown() {
	zend_hash_destroy(&UOPZ(opcodes));
	zend_hash_destroy(&UOPZ(modifiers));
	zend_hash_destroy(&UOPZ(optypes));
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
static inline void uopz_disassemble_arginfo(zend_op_array *function, zval *disassembly) {
	zval result;
	uint32_t it = 0;

	if (!function->num_args && !UOPZ_HAS_RETURN_TYPE(function)) {
		return;
	}

	if (function->num_args)
		add_assoc_long(disassembly, "nargs", function->num_args);
	if (function->required_num_args)
		add_assoc_long(disassembly, "rnargs", function->required_num_args);

	array_init(&result);
	if (UOPZ_HAS_RETURN_TYPE(function)) {
		zval ret;
		zend_arg_info *return_type = (function->arg_info - 1);

		array_init(&ret);
		if (return_type->class_name) {
			add_assoc_str(&ret, "class", zend_string_copy(return_type->class_name));
		} else if (return_type->type_hint != IS_UNDEF) {
			add_assoc_str(&ret, "type", uopz_disassemble_type_name(return_type->type_hint));
		}

		zend_hash_index_add(Z_ARRVAL(result), -1, &ret);		
	}

	while (it < function->num_args) {
		zval arg;

		array_init(&arg);
		add_assoc_str(&arg, "name", zend_string_copy(function->arg_info[it].name));
		if (function->arg_info[it].class_name) {
			add_assoc_str(&arg, "class", zend_string_copy(function->arg_info[it].class_name));
		} else if (function->arg_info[it].type_hint != IS_UNDEF) {
			add_assoc_str(&arg, "type", uopz_disassemble_type_name(function->arg_info[it].type_hint));
		}
		add_assoc_bool(&arg, "reference", function->arg_info[it].pass_by_reference);
		add_assoc_bool(&arg, "null", function->arg_info[it].allow_null);
		add_assoc_bool(&arg, "variadic", function->arg_info[it].is_variadic);

		zend_hash_next_index_insert(Z_ARRVAL(result), &arg);
		it++;
	}

	zend_hash_str_add(Z_ARRVAL_P(disassembly), "arginfo", sizeof("arginfo") - 1, &result);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_operand(char *name, size_t nlen, zend_op_array *op_array, zend_op *opline, zend_uchar op_type, znode_op *op, zend_bool jmp, zval *disassembly) {
	zval result;

	if (op_type == IS_UNUSED && !jmp && !op->num)
		return;

	array_init(&result);

#define IS_OP1 (op == &opline->op1) 
#define IS_OP2 (op == &opline->op2)
#define IS_RESULT (op == &opline->result)

	if (jmp) {
		ZEND_PASS_TWO_UNDO_JMP_TARGET(op_array, opline, *op);
		add_assoc_long(&result, "jmp", op->opline_num);
		ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, *op);
	} else if (op_type & IS_CONST) {
		ZEND_PASS_TWO_UNDO_CONSTANT(op_array, *op);
		add_assoc_str(&result,  "type", uopz_disassemble_optype(IS_CONST));
		add_assoc_long(&result, "num", (int32_t) op->num);
		ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, *op);
	} else if(op_type & (IS_TMP_VAR|IS_VAR)) {
		if (op_type & IS_TMP_VAR)
			add_assoc_str(&result, "type", uopz_disassemble_optype(IS_TMP_VAR));
		else add_assoc_str(&result, "type", uopz_disassemble_optype(IS_VAR));
		add_assoc_long(&result, "num", (EX_VAR_TO_NUM(op->num) - op_array->last_var));
	} else if (op_type & IS_CV) {
		add_assoc_str(&result, "type", uopz_disassemble_optype(IS_CV));
		add_assoc_long(&result, "num", EX_VAR_TO_NUM(op->num));
	} else if (op_type & IS_UNUSED) {
		add_assoc_str(&result, "type", uopz_disassemble_optype(IS_UNUSED));

		switch (opline->opcode) {
			case ZEND_RECV_VARIADIC:
			case ZEND_RECV:
			case ZEND_VERIFY_RETURN_TYPE:
				if (IS_OP2) {
					add_assoc_long(&result, "num", (int32_t) op->num);
					break;
				}

			case ZEND_SEND_VAR_EX:
				if (IS_RESULT) {
					add_assoc_long(&result, "num", EX_VAR_TO_NUM(op->num));
					break;
				}

			default:
				add_assoc_long(&result, "num", op->num);
		}
	}

#undef IS_OP1
#undef IS_OP2
#undef IS_RESULT

	if (op_type & EXT_TYPE_UNUSED) {
		add_assoc_bool(&result, "ext-type-unused", 1);
	}

	zend_hash_str_add(Z_ARRVAL_P(disassembly), name, nlen, &result);
} /* }}} */

/* {{{ */
static inline void upoz_disassemble_extended_value(zend_op_array *op_array, zend_op *opline, zval *disassembly) {
	switch (opline->opcode) {
		case ZEND_TYPE_CHECK:
		case ZEND_CAST:
			add_assoc_str(disassembly, "type", uopz_disassemble_type_name(opline->extended_value));
		break;

		case ZEND_JMPZNZ:
		case ZEND_FE_FETCH_R:
		case ZEND_FE_FETCH_RW:
			add_assoc_long(disassembly, "ext", ZEND_OFFSET_TO_OPLINE_NUM(op_array, opline, opline->extended_value));
		break;

		case ZEND_FETCH_CLASS_NAME:
			switch (opline->extended_value) {
				case ZEND_FETCH_CLASS_SELF:
					add_assoc_str(disassembly, "fetch", zend_string_init(ZEND_STRL("self"), 0));
				break;

				case ZEND_FETCH_CLASS_STATIC:
					add_assoc_str(disassembly, "fetch", zend_string_init(ZEND_STRL("static"), 0));
				break;

				case ZEND_FETCH_CLASS_PARENT:
					add_assoc_str(disassembly, "fetch", zend_string_init(ZEND_STRL("parent"), 0));
				break;
				
				case ZEND_FETCH_CLASS_DEFAULT:
					add_assoc_str(disassembly, "fetch", zend_string_init(ZEND_STRL("default"), 0));
				break;
			}
		break;

		default:
			/* TODO(krakjoe) make more of these human friendly */
			if (opline->extended_value)
				add_assoc_long(disassembly, "ext", opline->extended_value);
	}
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_lineno(zend_op *opline, zval *disassembly) {
	if (opline->lineno)
		add_assoc_long(disassembly, "lineno", (zend_long) opline->lineno);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_opcodes(zend_op_array *op_array, zval *disassembly) {
	zend_op *opline = op_array->opcodes;
	zend_op *end    = opline + op_array->last;
	zval result;

	array_init(&result);
	
	opline = op_array->opcodes;
	end    = opline + op_array->last;

	while (opline < end) {
		zval opcode;
		zend_string *name = zend_hash_index_find_ptr(&UOPZ(opcodes), opline->opcode);

		array_init(&opcode);
		if (name) {
			add_assoc_str(&opcode, "opcode", zend_string_copy(name));
		} else add_assoc_long(&opcode, "opcode", opline->opcode);

		switch (opline->opcode) {
			case ZEND_JMP:
			case ZEND_FAST_CALL:
			case ZEND_DECLARE_ANON_CLASS:
			case ZEND_DECLARE_ANON_INHERITED_CLASS:
				uopz_disassemble_operand(ZEND_STRL("op1"), op_array, opline, opline->op1_type, &opline->op1, 1, &opcode);
				uopz_disassemble_operand(ZEND_STRL("op2"), op_array, opline, opline->op2_type, &opline->op2, 0, &opcode);
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
				uopz_disassemble_operand(ZEND_STRL("op1"), op_array, opline, opline->op1_type, &opline->op1, 0, &opcode);
				uopz_disassemble_operand(ZEND_STRL("op2"), op_array, opline, opline->op2_type, &opline->op2, 1, &opcode);
			break;

			default:
				uopz_disassemble_operand(ZEND_STRL("op1"), op_array, opline, opline->op1_type, &opline->op1, 0, &opcode);
				uopz_disassemble_operand(ZEND_STRL("op2"), op_array, opline, opline->op2_type, &opline->op2, 0, &opcode);
		}
		
		uopz_disassemble_operand(ZEND_STRL("result"), op_array, opline, opline->result_type, &opline->result, 0, &opcode);
		upoz_disassemble_extended_value(op_array, opline, &opcode);
		uopz_disassemble_lineno(opline, &opcode);

		zend_hash_next_index_insert(Z_ARRVAL(result), &opcode);
		opline++;
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
		add_assoc_zval(
			disassembly, "static", &tmp);
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

	if (flags & ZEND_ACC_GENERATOR)
		add_next_index_str(&result, uopz_disassemble_modifier_name(ZEND_ACC_GENERATOR));

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
static inline void uopz_disassemble_name(zend_op_array *function, zval *disassembly) {
	if (function->function_name)
		add_assoc_str(disassembly, "name",  zend_string_copy(function->function_name));
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_scope(zend_op_array *function, zval *disassembly) {
	if (function->scope)
		add_assoc_str(disassembly, "scope", zend_string_copy(function->scope->name));
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_misc(zend_op_array *function, zval *disassembly) {
	if (function->filename)
		add_assoc_str(disassembly, "file", function->filename);
	if (function->line_start)
		add_assoc_long(disassembly, "start", function->line_start);
	if (function->line_end)
		add_assoc_long(disassembly, "end", function->line_end);
	if (function->doc_comment)
		add_assoc_str(disassembly, "comment", function->doc_comment);
	if (function->cache_size)
		add_assoc_long(disassembly, "cache", function->cache_size);
	if (function->T)
		add_assoc_long(disassembly, "T", function->T);
} /* }}} */

/* {{{ */
static inline void uopz_disassemble_function(zend_op_array *function, zval *disassembly) {
	uopz_disassemble_name(function, disassembly);
	uopz_disassemble_scope(function, disassembly);
	uopz_disassemble_flags(function->fn_flags, disassembly);
	uopz_disassemble_arginfo(function, disassembly);
	uopz_disassemble_opcodes(function, disassembly);
	uopz_disassemble_vars(function->vars, function->last_var, disassembly);
	uopz_disassemble_literals(function->literals, function->last_literal, disassembly);
	uopz_disassemble_statics(function->static_variables, disassembly);
	uopz_disassemble_brk(function->brk_cont_array, function->last_brk_cont, disassembly);
	uopz_disassemble_try(function->try_catch_array, function->last_try_catch, disassembly);
	uopz_disassemble_misc(function, disassembly);
} /* }}} */

#endif
