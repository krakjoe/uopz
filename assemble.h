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

#define UOPZ_CONST_NUM(c) 		((c > 0L ? (c) * sizeof(zval) : c))
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

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(flags), flag) {
		uopz_assemble_flag(assembled, flag);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline zend_uchar uopz_assemble_type_hint(zval *disassembly) {
	if (Z_TYPE_P(disassembly) == IS_STRING) {
		if (UOPZ_ZVAL_MATCH(disassembly, "int")) {
			return IS_LONG;
		} else if (UOPZ_ZVAL_MATCH(disassembly, "double")) {
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
	
	if (!info) {
		return;
	}
	
	assembled->arg_info = arg_info =
		ecalloc(sizeof(zend_arg_info), zend_hash_num_elements(Z_ARRVAL_P(info)));
	
	if (zend_hash_index_exists(Z_ARRVAL_P(info), -1)) {
		assembled->fn_flags |= ZEND_ACC_HAS_RETURN_TYPE;
	}

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

	if (UOPZ_HAS_RETURN_TYPE(assembled)) {
		assembled->arg_info++;
	}
} /* }}} */

/* {{{ */
static inline void uopz_assemble_operand(zend_op *opline, znode_op *operand, zend_uchar *type, uint32_t last_var, zval *disassembly) {
	zval *op = NULL;

	if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("cv")))) {
		*type = IS_CV;
		operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op));
	} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("tmp")))) {
		*type = IS_TMP_VAR;
		operand->num = (uintptr_t) (ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op) + last_var));
	} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("var")))) {
		*type = IS_VAR;
		operand->num = (uintptr_t) ZEND_CALL_VAR_NUM(NULL, Z_LVAL_P(op));
	} else if ((op = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("constant")))) {
		*type = IS_CONST;
		operand->num = UOPZ_CONST_NUM(Z_LVAL_P(op));
	}

#if 0
	php_printf("a: %d: %d -> %d\n", *type, Z_LVAL_P(op), operand->num);
#endif
} /* }}} */

/* {{{ */
static inline uint32_t uopz_assemble_opcode_num(zval *disassembly) {
	zval *opcode = NULL;
	zend_ulong opnum;

	ZEND_HASH_FOREACH_NUM_KEY_VAL(&UOPZ(opcodes), opnum, opcode) {
		if (zend_string_equals(Z_STR_P(opcode), Z_STR_P(disassembly))) {
			return opnum;
		}
	} ZEND_HASH_FOREACH_END();
	return 0;
} /* }}} */

/* {{{ */
static inline void uopz_assemble_extended_value(zend_op *assembled, zval *disassembly) {
	switch (assembled->opcode) {
		case ZEND_FETCH_UNSET:
		case ZEND_FETCH_RW:
		case ZEND_FETCH_W:
		case ZEND_FETCH_R: {
			zval *fetch = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("fetch"));

			if (UOPZ_ZVAL_MATCH(fetch, "static")) {
				assembled->extended_value = ZEND_FETCH_STATIC;
			} else if (UOPZ_ZVAL_MATCH(fetch, "global")) {
				assembled->extended_value = ZEND_FETCH_GLOBAL;
			} else if (UOPZ_ZVAL_MATCH(fetch, "local")) {
				assembled->extended_value = ZEND_FETCH_LOCAL;
			}
		} break;

		case ZEND_CAST: {
			zval *type = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("type"));
			
			if (type) {
				assembled->extended_value = uopz_assemble_type_hint(type);
			}
		} break;
	}
} /* }}} */

/* {{{ */
static inline void uopz_assemble_opcode(zend_op_array *assembled, uint32_t it, uint32_t last_var, zval *disassembly) {
	zval *opcode = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("opcode"));
	zval *op1 = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("op1"));
	zval *op2 = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("op2"));
	zval *result = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("result"));

	assembled->opcodes[it].opcode = uopz_assemble_opcode_num(opcode);
	
	if (op1)
		uopz_assemble_operand(&assembled->opcodes[it], &assembled->opcodes[it].op1, &assembled->opcodes[it].op1_type, last_var, op1);
	else assembled->opcodes[it].op1_type = IS_UNUSED;

	if (op2)
		uopz_assemble_operand(&assembled->opcodes[it], &assembled->opcodes[it].op2, &assembled->opcodes[it].op2_type, last_var, op2);
	else assembled->opcodes[it].op2_type = IS_UNUSED;

	if (result)
		uopz_assemble_operand(&assembled->opcodes[it], &assembled->opcodes[it].result, &assembled->opcodes[it].result_type, last_var, result);
	else assembled->opcodes[it].result_type = IS_UNUSED;

	uopz_assemble_extended_value(&assembled->opcodes[it], disassembly);

	zend_vm_set_opcode_handler(&assembled->opcodes[it]);
} /* }}} */

/* {{{ */
static inline void uopz_assemble_opcodes(zend_op_array *assembled, zval *disassembly) {
	zval *opcodes  = zend_hash_str_find(
		Z_ARRVAL_P(disassembly), ZEND_STRL("opcodes"));
	zval *opcode   = NULL;
	uint32_t it   = 0;

	assembled->last = zend_hash_num_elements(Z_ARRVAL_P(opcodes));
	assembled->opcodes = 
		(zend_op*) ecalloc(sizeof(zend_op), assembled->last);
	
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(opcodes), opcode) {
		uopz_assemble_opcode(assembled, it++, assembled->last_var, opcode);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void uopz_assemble_vars(zend_op_array *assembled, zval *disassembly) {
	zval *vars = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("vars"));
	zval *var = NULL;
	zend_ulong idx = 0;

	assembled->last_var = zend_hash_num_elements(Z_ARRVAL_P(vars));
	assembled->vars = ecalloc(sizeof(zend_string*), assembled->last_var);

	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(vars), idx, var) {
		assembled->vars[idx] = zend_string_copy(Z_STR_P(var));
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void uopz_assemble_literals(zend_op_array *assembled, zval *disassembly) {
	zval *literals = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("literals"));
	zval *literal  = NULL;
	zend_ulong idx = 0;

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

	assembled->static_variables = zend_array_dup(Z_ARRVAL_P(statics));
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
	uopz_assemble_opcodes(assembled, disassembly);
	uopz_assemble_literals(assembled, disassembly);
	uopz_assemble_statics(assembled, disassembly);

	return (zend_function*) assembled;
} /* }}} */

#undef UOPZ_CONST_NUM
#endif
