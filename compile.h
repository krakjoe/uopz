/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2012 - 2014                                |
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

#ifndef HAVE_UOPZ_COMPILE_H
#define HAVE_UOPZ_COMPILE_H

#define CONSTANT_EX(op_array, op) \
	(op_array)->literals[op].constant

#define SET_NODE(ops, target, src) do { \
		target ## _type = IS_CONST; \
		target.constant = zend_add_literal(ops, &src TSRMLS_CC); \
	} while (0)

#define GET_NODE(op_array, target, src) do { \
		(target)->op_type = src ## _type; \
		(target)->u.op = src; \
		(target)->EA = 0; \
	} while (0)

#define CALCULATE_LITERAL_HASH(op_array, num) do { \
		if (IS_INTERNED(Z_STRVAL(CONSTANT_EX(op_array, num)))) { \
			Z_HASH_P(&CONSTANT_EX(op_array, num)) = INTERNED_HASH(Z_STRVAL(CONSTANT_EX(op_array, num))); \
		} else { \
			Z_HASH_P(&CONSTANT_EX(op_array, num)) = zend_hash_func(Z_STRVAL(CONSTANT_EX(op_array, num)), Z_STRLEN(CONSTANT_EX(op_array, num))+1); \
		} \
	} while (0)

#define GET_CACHE_SLOT(op_array, literal) do { \
		op_array->literals[literal].cache_slot = op_array->last_cache_slot++; \
		if ((op_array->fn_flags & ZEND_ACC_INTERACTIVE) && op_array->run_time_cache) { \
			op_array->run_time_cache = erealloc(op_array->run_time_cache, op_array->last_cache_slot * sizeof(void*)); \
			op_array->run_time_cache[op_array->last_cache_slot - 1] = NULL; \
		} \
	} while (0)
	
#if PHP_VERSION_ID < 50500
#define GET_OP1(as) do {\
	if ((OPLINE->op1_type != IS_UNUSED) &&\
		(op1 = zend_get_zval_ptr(\
			OPLINE->op1_type, &OPLINE->op1, execute_data->Ts, &free_op1, as TSRMLS_CC))) {\
		fci.params[0] = &op1;\
	} else {\
		fci.params[0] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)

#define GET_OP2(as) do {\
	if ((OPLINE->op2_type != IS_UNUSED) &&\
		(op2 = zend_get_zval_ptr(\
			OPLINE->op2_type, &OPLINE->op2, execute_data->Ts, &free_op2, as TSRMLS_CC))) {\
		fci.params[1] = &op2;\
	} else {\
		fci.params[1] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)
#else
#define GET_OP1(as) do {\
	if ((OPLINE->op1_type != IS_UNUSED) &&\
		(op1 = zend_get_zval_ptr(\
			OPLINE->op1_type, &OPLINE->op1, execute_data, &free_op1, as TSRMLS_CC))) {\
		fci.params[0] = &op1;\
	} else {\
		fci.params[0] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)

#define GET_OP2(as) do {\
	if ((OPLINE->op2_type != IS_UNUSED) &&\
		(op2 = zend_get_zval_ptr(\
			OPLINE->op2_type, &OPLINE->op2, execute_data, &free_op2, as TSRMLS_CC))) {\
		fci.params[1] = &op2;\
	} else {\
		fci.params[1] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)
#endif
	
/* Common part of zend_add_literal and zend_append_individual_literal */
static inline void zend_insert_literal(zend_op_array *op_array, const zval *zv, int literal_position TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE_P(zv) == IS_STRING || Z_TYPE_P(zv) == IS_CONSTANT) {
		zval *z = (zval*)zv;
		Z_STRVAL_P(z) = (char*)zend_new_interned_string(Z_STRVAL_P(zv), Z_STRLEN_P(zv) + 1, 1 TSRMLS_CC);
	}
	CONSTANT_EX(op_array, literal_position) = *zv;
	Z_SET_REFCOUNT(CONSTANT_EX(op_array, literal_position), 2);
	Z_SET_ISREF(CONSTANT_EX(op_array, literal_position));
	op_array->literals[literal_position].hash_value = 0;
	op_array->literals[literal_position].cache_slot = -1;
}
/* }}} */

#if PHP_VERSION_ID > 50599
/* {{{ */
static inline void zend_push_function_call_entry(zend_function *fbc TSRMLS_DC) /* {{{ */
{
    zend_function_call_entry fcall = { fbc };
    zend_stack_push(&CG(function_call_stack), &fcall, sizeof(zend_function_call_entry));
}
/* }}} */
#else
/* {{{ */
static inline void zend_push_function_call_entry(zend_function *fbc TSRMLS_DC) /* {{{ */
{
    zend_stack_push(&CG(function_call_stack), &fbc, sizeof(zend_function*));
}
/* }}} */
#endif

/* {{{ */
int zend_add_literal(zend_op_array *op_array, const zval *zv TSRMLS_DC) /* {{{ */
{
	int i = op_array->last_literal;
	op_array->last_literal++;
	if (i >= CG(context).literals_size) {
		while (i >= CG(context).literals_size) {
			CG(context).literals_size += 16; /* FIXME */
		}
		op_array->literals = (zend_literal*)erealloc(op_array->literals, CG(context).literals_size * sizeof(zend_literal));
	}
	zend_insert_literal(op_array, zv, i TSRMLS_CC);
	return i;
}
/* }}} */

/* {{{ */
static inline void php_uopz_realloc(zend_op_array *op_array, zend_uint size) {
	op_array->opcodes = erealloc(op_array->opcodes, size * sizeof(zend_op));
} /* }}} */

/* {{{ */
static inline void php_uopz_init(zend_op *op TSRMLS_DC) {
	memset(op, 0, sizeof(zend_op));
	op->lineno = CG(zend_lineno);
	SET_UNUSED(op->result);
} /* }}} */

#if PHP_VERSION_ID < 50500
/* {{{ */
static inline zend_uint php_uopz_temp_var(zend_op_array *op_array TSRMLS_DC) {
	 return (op_array->T)++ * ZEND_MM_ALIGNED_SIZE(sizeof(temp_variable));
} /* }}} */
#else
/* {{{ */
static inline zend_uint php_uopz_temp_var(zend_op_array *op_array TSRMLS_DC) {
	 return (zend_uint)(zend_uintptr_t)EX_TMP_VAR_NUM(0, (op_array->T)++);
} /* }}} */
#endif

/* {{{ */
static inline zend_op* php_uopz_next(zend_op_array *op_array TSRMLS_DC) {
	zend_uint next_op_num = op_array->last++;
	zend_op *next_op = NULL;
	
	if (next_op_num >= CG(context).opcodes_size) {
		if (op_array->fn_flags & ZEND_ACC_INTERACTIVE) {
			zend_printf("Ran out of opcode space in interactive mode !");
			zend_bailout();
		}
		
		CG(context).opcodes_size *= 4;
		php_uopz_realloc
			(op_array, CG(context).opcodes_size);
	}
	
	next_op = &op_array->opcodes[next_op_num];
	php_uopz_init(next_op TSRMLS_CC);
	return next_op;
} /* }}} */

/* {{{ */
static inline zend_uint php_uopz_next_num(zend_op_array *op_array) {
	return op_array->last;
} /* }}} */

#endif
