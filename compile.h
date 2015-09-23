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

#ifndef HAVE_UOPZ_COMPILE_H
#define HAVE_UOPZ_COMPILE_H

#define SET_NODE(ops, target, src) do { \
		target ## _type = IS_CONST; \
		if ((src)->op_type == IS_CONST) {\
			target.constant = zend_add_literal(ops, &src TSRMLS_CC); \
		} else {\
			(target) = (src)->u.op;\
		}\
	} while (0)

#define GET_NODE(op_array, target, src) do { \
		(target)->op_type = src ## _type; \
		if ((target)->op_type == IS_CONST) {\
			ZVAL_COPY_VALUE(&(target)->u.constant, CT_CONSTANT(src));\
		} else { \
			(target)->u.op = src; \
		} \
	} while (0)

#define GET_OP1_IN(as, i) do {\
	if ((OPLINE->op1_type != IS_UNUSED) &&\
		(op1 = zend_get_zval_ptr(\
			OPLINE->op1_type, &OPLINE->op1, execute_data, &free_op1, as TSRMLS_CC))) {\
		ZVAL_COPY(&fci.params[i], op1); \
	} else { \
		ZVAL_COPY(&fci.params[i], &EG(uninitialized_zval)); \
	}\
} while(0)
#define GET_OP1(as) GET_OP1_IN(as, 0)

#define GET_OP2_IN(as, i) do {\
	if ((OPLINE->op2_type != IS_UNUSED) &&\
		(op2 = zend_get_zval_ptr(\
			OPLINE->op2_type, &OPLINE->op2, execute_data, &free_op2, as TSRMLS_CC))) {\
		fci.params[i] = &op2;\
	} else {\
		fci.params[i] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)
#define GET_OP2(as) GET_OP2_IN(as, 1)

static inline void zend_alloc_cache_slot(uint32_t literal) {
    zend_op_array *op_array = CG(active_op_array);
    Z_CACHE_SLOT(op_array->literals[literal]) = op_array->cache_size;
    op_array->cache_size += sizeof(void*);
}

#define POLYMORPHIC_CACHE_SLOT_SIZE 2

static inline void zend_alloc_polymorphic_cache_slot(uint32_t literal) {
    zend_op_array *op_array = CG(active_op_array);
    Z_CACHE_SLOT(op_array->literals[literal]) = op_array->cache_size;
    op_array->cache_size += POLYMORPHIC_CACHE_SLOT_SIZE * sizeof(void*);
}

/* Common part of zend_add_literal and zend_append_individual_literal */
static inline void zend_insert_literal(zend_op_array *op_array, zval *zv, int literal_position) /* {{{ */
{
    if (Z_TYPE_P(zv) == IS_STRING || Z_TYPE_P(zv) == IS_CONSTANT) {
        zend_string_hash_val(Z_STR_P(zv));
        Z_STR_P(zv) = zend_new_interned_string(Z_STR_P(zv));
        if (ZSTR_IS_INTERNED(Z_STR_P(zv))) {
            Z_TYPE_FLAGS_P(zv) &= ~ (IS_TYPE_REFCOUNTED | IS_TYPE_COPYABLE);
        }
    }
    ZVAL_COPY_VALUE(CT_CONSTANT_EX(op_array, literal_position), zv);
    Z_CACHE_SLOT(op_array->literals[literal_position]) = -1;
}
/* }}} */

/* {{{ */
static inline void php_uopz_realloc(zend_op_array *op_array, uint32_t size) {
	op_array->opcodes = erealloc(op_array->opcodes, size * sizeof(zend_op));
} /* }}} */

/* {{{ */
static inline void php_uopz_init(zend_op *op TSRMLS_DC) {
	memset(op, 0, sizeof(zend_op));
	op->lineno = CG(zend_lineno);
	SET_UNUSED(op->result);
} /* }}} */

/* {{{ */
static inline zend_op* php_uopz_next(zend_op_array *op_array TSRMLS_DC) {
	uint32_t next_op_num = op_array->last++;
	zend_op *next_op = NULL;
	
	if (next_op_num >= CG(context).opcodes_size) {
		CG(context).opcodes_size *= 4;
		php_uopz_realloc
			(op_array, CG(context).opcodes_size);
	}
	
	next_op = &op_array->opcodes[next_op_num];
	php_uopz_init(next_op TSRMLS_CC);
	return next_op;
} /* }}} */

/* {{{ */
static inline uint32_t php_uopz_next_num(zend_op_array *op_array) {
	return op_array->last;
} /* }}} */

#endif
