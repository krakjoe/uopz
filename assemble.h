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
#ifndef HAVE_UOPZ_ASSEMBLE_H
#define HAVE_UOPZ_ASSEMBLE_H

#define UOPZ_ZVAL_MATCH(z, c)	(Z_TYPE_P(z) == IS_STRING && Z_STRLEN_P(z) == sizeof(c)-1 && memcmp(Z_STRVAL_P(z), c, Z_STRLEN_P(z)) == SUCCESS)
#define UOPZ_STR_MATCH(s, c)	((s)->len == sizeof(c)-1 && memcmp((s)->val, c, (s)->len) == SUCCESS)

/* {{{ */
static inline void uopz_assemble_name(zend_op_array *assembled, zval *disassembly) {
	zval *name = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("name"));
	
	if (!name)
		return;	

	assembled->function_name = zend_string_copy(Z_STR_P(name));
} /* }}} */

/* {{{ */
static inline void uopz_assemble_flag(zend_op_array *assembled, zval *disassembly) {
	zend_string *name = Z_STR_P(disassembly);

	if (UOPZ_STR_MATCH(name, "final")) {
		assembled->fn_flags |= ZEND_ACC_FINAL;
	} else if (UOPZ_STR_MATCH(name, "static")) {
		assembled->fn_flags |= ZEND_ACC_STATIC;
	} else if (UOPZ_STR_MATCH(name, "reference")) {
		assembled->fn_flags |= ZEND_ACC_RETURN_REFERENCE;
	} else if (UOPZ_STR_MATCH(name, "protected")) {
		assembled->fn_flags |= ZEND_ACC_PROTECTED;
	} else if (UOPZ_STR_MATCH(name, "private")) {
		assembled->fn_flags |= ZEND_ACC_PRIVATE;
	} else if (UOPZ_STR_MATCH(name, "public")) {
		assembled->fn_flags |= ZEND_ACC_PUBLIC;
	}
} /* }}} */

/* {{{ */
static inline void uopz_assemble_flags(zend_op_array *assembled, zval *disassembly) {
	zval *flags = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("flags"));
	zval *flag  = NULL;

	assembled->fn_flags |= ZEND_ACC_DONE_PASS_TWO;

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(flags), flag) {
		uopz_assemble_flag(assembled, flag);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline zend_uchar uopz_assemble_type_hint(zval *disassembly) {
	if (Z_TYPE_P(disassembly) == IS_STRING) {
		if (UOPZ_ZVAL_MATCH(disassembly, "int")) {
			return IS_LONG;
		} else if (UOPZ_ZVAL_MATCH(disassembly, "float")) {
			return IS_DOUBLE;
		} else if (UOPZ_ZVAL_MATCH(disassembly, "array")) {
			return IS_ARRAY;
		} else if (UOPZ_ZVAL_MATCH(disassembly, "callable")) {
			return IS_CALLABLE;
		} else if (UOPZ_ZVAL_MATCH(disassembly, "string")) {
			return IS_STRING;
		}
	} 

	return IS_UNDEF;
} /* }}} */

/* {{{ */
static inline void uopz_assemble_arginfo(zend_op_array *assembled, zval *disassembly) {
	zval *info = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("nargs"));
	zval *arginfo = NULL;
	zend_long idx = 0;
	zend_arg_info *arg_info = NULL;

	if (info)
		assembled->num_args = Z_LVAL_P(info);
	
	info = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("rnargs"));
	
	if (info)
		assembled->required_num_args = Z_LVAL_P(info);

	info = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("arginfo"));
	
	if (!info)
		return;
	
	assembled->arg_info = arg_info =
		ecalloc(sizeof(zend_arg_info), zend_hash_num_elements(Z_ARRVAL_P(info)));

	if (zend_hash_index_exists(Z_ARRVAL_P(info), -1))
		assembled->fn_flags |= ZEND_ACC_HAS_RETURN_TYPE;

	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(info), idx, arginfo) {
		zval *name = zend_hash_str_find(
			Z_ARRVAL_P(arginfo), ZEND_STRL("name"));
		zval *clazz = zend_hash_str_find(
			Z_ARRVAL_P(arginfo), ZEND_STRL("class"));
		zval *reference = zend_hash_str_find(
			Z_ARRVAL_P(arginfo), ZEND_STRL("reference"));
		zval *null = zend_hash_str_find(
			Z_ARRVAL_P(arginfo), ZEND_STRL("null"));
		zval *variadic = zend_hash_str_find(
			Z_ARRVAL_P(arginfo), ZEND_STRL("variadic"));
		zval *type = zend_hash_str_find(
			Z_ARRVAL_P(arginfo), ZEND_STRL("type"));

		if (name)
			arg_info->name = zend_string_copy(Z_STR_P(name));
		if (clazz)
			arg_info->class_name = zend_string_copy(Z_STR_P(clazz));
		if (type)
			arg_info->type_hint = uopz_assemble_type_hint(type);
		if (reference)
			arg_info->pass_by_reference = zend_is_true(reference);
		if (null)
			arg_info->allow_null = zend_is_true(null);
		if (variadic)
			arg_info->is_variadic = zend_is_true(variadic);
		arg_info++;
	} ZEND_HASH_FOREACH_END();

	if (UOPZ_HAS_RETURN_TYPE(assembled))
		assembled->arg_info++;
} /* }}} */

/* {{{ */
static inline void uopz_assemble_operand(zend_op_array *op_array, zend_op *opline, znode_op *operand, zend_uchar *type, zend_bool jmp, zval *disassembly) {
	zval *op = NULL;

	if (!disassembly) {
		*type = IS_UNUSED;
		return;	
	}
	
	if (jmp) {
		*type = IS_UNUSED;
		if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("jmp")))) {
			operand->opline_num = Z_LVAL_P(op);
			ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, *operand);
		}
	} else {
		if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("constant")))) {
			*type = IS_CONST;
			operand->num = Z_LVAL_P(op);
			ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, *operand);
		} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("cv")))) {
			*type = IS_CV;
			operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op));
		} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("tmp")))) {
			*type = IS_TMP_VAR;
			operand->num = (uintptr_t) (ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op) + op_array->last_var));
		} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("var")))) {
			*type = IS_VAR;
			operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op));
		} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("unused")))) {
			*type = IS_UNUSED;
			operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op));
		} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("ext")))) {
			zval *ext = NULL;

			*type |= EXT_TYPE_UNUSED;

			if ((ext = zend_hash_str_find(Z_ARRVAL_P(op), ZEND_STRL("constant")))) {
				*type |= IS_CONST;
				operand->num = Z_LVAL_P(ext);
				ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, *operand);
			} else if ((ext = zend_hash_str_find(Z_ARRVAL_P(op), ZEND_STRL("cv")))) {
				*type |= IS_CV;
				operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(ext));
			} else if ((ext = zend_hash_str_find(Z_ARRVAL_P(op), ZEND_STRL("tmp")))) {
				*type |= IS_TMP_VAR;
				operand->num = (uintptr_t) (ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(ext) + op_array->last_var));
			} else if ((ext = zend_hash_str_find(Z_ARRVAL_P(op), ZEND_STRL("var")))) {
				*type |= IS_VAR;
				operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(ext));
			} else if ((ext = zend_hash_str_find(Z_ARRVAL_P(op), ZEND_STRL("unused")))) {
				*type |= IS_UNUSED;
				operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(ext));
			}
		}
	}
} /* }}} */

/* {{{ */
static inline uint32_t uopz_assemble_opcode_num(zval *disassembly) {
	zval *opcode = NULL;
	zend_long opnum = 0;

	if (Z_TYPE_P(disassembly) == IS_LONG)
		return Z_LVAL_P(disassembly);

	ZEND_HASH_FOREACH_NUM_KEY_VAL(&UOPZ(opcodes), opnum, opcode) {
		if (zend_string_equals(Z_STR_P(opcode), Z_STR_P(disassembly)))
			return opnum;
	} ZEND_HASH_FOREACH_END();
	return 0;
} /* }}} */

/* {{{ */
static inline void uopz_assemble_extended_value(zend_op *assembled, zval *disassembly) {
	switch (assembled->opcode) {
		case ZEND_CAST: {
			zval *type = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("type"));
			if (type)
				assembled->extended_value = uopz_assemble_type_hint(type);
		} break;

		default: {
			zval *ext = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("ext"));
			if (ext && Z_TYPE_P(ext) == IS_LONG)
				assembled->extended_value = Z_LVAL_P(ext);
		}
	}
} /* }}} */

/* {{{ */
static inline void uopz_assemble_opcode(zend_op_array *assembled, uint32_t it, zval *disassembly) {
	zval *opcode = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("opcode"));
	zval *op1 = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("op1"));
	zval *op2 = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("op2"));
	zval *result = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("result"));

	assembled->opcodes[it].opcode = uopz_assemble_opcode_num(opcode);

	switch (assembled->opcodes[it].opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
			uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].op1, &assembled->opcodes[it].op1_type, 1, op1);
			uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].op2, &assembled->opcodes[it].op2_type, 0, op2);
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
			uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].op1, &assembled->opcodes[it].op1_type, 0, op1);
			uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].op2, &assembled->opcodes[it].op2_type, 1, op2);
		break;

		default:
			uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].op1, &assembled->opcodes[it].op1_type, 0, op1);
			uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].op2, &assembled->opcodes[it].op2_type, 0, op2);
	}

	uopz_assemble_operand(assembled, &assembled->opcodes[it], &assembled->opcodes[it].result, &assembled->opcodes[it].result_type, 0, result);
	uopz_assemble_extended_value(&assembled->opcodes[it], disassembly);

	zend_vm_set_opcode_handler(&assembled->opcodes[it]);
} /* }}} */

/* {{{ */
static inline void uopz_assemble_opcodes(zend_op_array *assembled, zval *disassembly) {
	zval *opcodes  = zend_hash_str_find(
		Z_ARRVAL_P(disassembly), ZEND_STRL("opcodes"));
	zval *opcode   = NULL;
	uint32_t it   = 0;

	if (!opcodes)
		return;

	if (Z_TYPE_P(opcodes) != IS_ARRAY)
		return;
	
	assembled->last = zend_hash_num_elements(Z_ARRVAL_P(opcodes));
	assembled->opcodes = 
		(zend_op*) ecalloc(sizeof(zend_op), assembled->last);
	
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(opcodes), opcode) {
		uopz_assemble_opcode(assembled, it++, opcode);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void uopz_assemble_vars(zend_op_array *assembled, zval *disassembly) {
	zval *vars = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("vars"));
	zval *var = NULL;
	zend_long idx = 0;

	if (!vars)
		return;

	if (Z_TYPE_P(vars) != IS_ARRAY)
		return;

	assembled->last_var = zend_hash_num_elements(Z_ARRVAL_P(vars));
	assembled->vars = ecalloc(sizeof(zend_string*), assembled->last_var);

	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(vars), idx, var) {
		assembled->vars[idx] = zend_string_dup(Z_STR_P(var), 0);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void uopz_assemble_literals(zend_op_array *assembled, zval *disassembly) {
	zval *literals = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("literals"));
	zval *literal  = NULL;
	zend_long idx  = 0;

	if (!literals)
		return;

	if (Z_TYPE_P(literals) != IS_ARRAY)
		return;

	assembled->last_literal = zend_hash_num_elements(Z_ARRVAL_P(literals));
	assembled->literals = ecalloc(sizeof(zval), assembled->last_literal);

	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(literals), idx, literal) {
		ZVAL_COPY(&assembled->literals[idx], literal);
	} ZEND_HASH_FOREACH_END();	
} /* }}} */

/* {{{ */
static inline void uopz_assemble_statics(zend_op_array *assembled, zval *disassembly) {
	zval *statics = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("static"));

	if (!statics)
		return;

	if (Z_TYPE_P(statics) != IS_ARRAY)
		return;

	assembled->static_variables = zend_array_dup(Z_ARRVAL_P(statics));
} /* }}} */

/* {{{ */
static inline void uopz_assemble_brk_element(zend_brk_cont_element *el, zval *disassembly) {
	zval *start = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("start"));
	zval *cont  = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("cont"));
	zval *brk   = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("brk"));
	zval *parent = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("parent"));

	if (start && Z_TYPE_P(start) == IS_LONG) {
		el->start = Z_LVAL_P(start);
	} else el->start = -1;
	
	if (cont && Z_TYPE_P(cont) == IS_LONG) {
		el->cont = Z_LVAL_P(cont);
	} else el->cont = -1;

	if (brk && Z_TYPE_P(brk) == IS_LONG) {
		el->brk = Z_LVAL_P(brk);
	} else el->brk = -1;

	if (parent && Z_TYPE_P(parent) == IS_LONG) {
		el->parent = Z_LVAL_P(parent);
	} else el->parent = -1;
} /* }}} */

/* {{{ */
static inline void uopz_assemble_brk(zend_op_array *assembled, zval *disassembly) {
	zval *brk = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("brk"));
	zval *el = NULL;
	zend_long idx = 0;

	if (!brk)
		return;

	if (Z_TYPE_P(brk) != IS_ARRAY)
		return;	
	
	assembled->last_brk_cont = zend_hash_num_elements(Z_ARRVAL_P(brk));	
	assembled->brk_cont_array = ecalloc(sizeof(zend_brk_cont_element), assembled->last_brk_cont);
	
	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(brk), idx, el) {
		if (Z_TYPE_P(el) != IS_ARRAY)
			continue;
		uopz_assemble_brk_element(&assembled->brk_cont_array[idx], el);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void uopz_assemble_try_element(zend_try_catch_element *el, zval *disassembly) {
	zval *try_op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("try"));
	zval *catch_op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("catch"));
	zval *finally_op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("finally"));
	zval *end_op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("end"));

	if (try_op && Z_TYPE_P(try_op) == IS_LONG)	
		el->try_op = Z_LVAL_P(try_op);
	if (catch_op && Z_TYPE_P(catch_op) == IS_LONG)
		el->catch_op = Z_LVAL_P(catch_op);
	if (finally_op && Z_TYPE_P(finally_op) == IS_LONG)
		el->finally_op = Z_LVAL_P(finally_op);
	if (end_op && Z_TYPE_P(end_op) == IS_LONG)
		el->finally_end = Z_LVAL_P(end_op);
} /* }}} */

/* {{{ */
static inline void uopz_assemble_try(zend_op_array *assembled, zval *disassembly) {
	zval *tri = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("try"));
	zval *el = NULL;
	zend_long idx = 0;
	
	if (!tri)
		return;

	if (Z_TYPE_P(tri) != IS_ARRAY)
		return;

	assembled->last_try_catch = zend_hash_num_elements(Z_ARRVAL_P(tri));
	assembled->try_catch_array = ecalloc(sizeof(zend_try_catch_element), assembled->last_try_catch);

	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(tri), idx, el) {
		if (Z_TYPE_P(el) != IS_ARRAY)
			continue;
		uopz_assemble_try_element(&assembled->try_catch_array[idx], el);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void uopz_assemble_misc(zend_op_array *assembled, zval *disassembly) {
	zval *filename = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("file"));
	zval *start    = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("start"));
	zval *end      = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("end"));
	zval *comment  = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("comment"));
	zval *cache    = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("cache"));
	zval *T		   = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("T"));

	if (filename && Z_TYPE_P(filename) == IS_STRING)
		assembled->filename = zend_string_copy(Z_STR_P(filename));
	if (comment && Z_TYPE_P(comment) == IS_STRING)
		assembled->doc_comment = zend_string_copy(Z_STR_P(comment));
	if (start && Z_TYPE_P(start) == IS_LONG)
		assembled->line_start = (uint32_t) Z_LVAL_P(start);
	if (end && Z_TYPE_P(end) == IS_LONG)
		assembled->line_end = (uint32_t) Z_LVAL_P(end);
	if (cache && Z_TYPE_P(cache) == IS_LONG)
		assembled->cache_size = Z_LVAL_P(cache);
	if (T && Z_TYPE_P(T) == IS_LONG)
		assembled->T = Z_LVAL_P(T);
} /* }}} */

/* {{{ */
static inline zend_function* uopz_assemble(zval *disassembly) {
	zend_op_array *assembled = 
		(zend_op_array*) zend_arena_alloc(&CG(arena), sizeof(zend_op_array));

	memset(assembled, 0, sizeof(zend_op_array));

	assembled->type = ZEND_USER_FUNCTION;
	assembled->refcount = (uint32_t*) emalloc(sizeof(uint32_t));
	*(assembled->refcount) = 1;
	
	uopz_assemble_name(assembled, disassembly);
	uopz_assemble_flags(assembled, disassembly);
	uopz_assemble_arginfo(assembled, disassembly);
	uopz_assemble_vars(assembled, disassembly);
	uopz_assemble_literals(assembled, disassembly);
	uopz_assemble_opcodes(assembled, disassembly);
	uopz_assemble_statics(assembled, disassembly);
	uopz_assemble_brk(assembled, disassembly);
	uopz_assemble_try(assembled, disassembly);
	uopz_assemble_misc(assembled, disassembly);

	return (zend_function*) assembled;
} /* }}} */
#endif
