/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2016                                       |
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
#ifndef HAVE_UOPZ_COPY
#define HAVE_UOPZ_COPY

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "copy.h"

#include <Zend/zend_vm.h>

/* {{{ */
static inline HashTable* uopz_copy_statics(HashTable *old) {
	return zend_array_dup(old);
} /* }}} */

/* {{{ */
static inline zend_string** uopz_copy_variables(zend_string **old, int end) {
	zend_string **variables = safe_emalloc(end, sizeof(zend_string*), 0);
	int it = 0;
	
	while (it < end) {
		variables[it] = zend_string_copy(old[it]);
		it++;
	}
	
	return variables;
} /* }}} */

/* {{{ */
static inline zend_try_catch_element* uopz_copy_try(zend_try_catch_element *old, int end) {	
	zend_try_catch_element *try_catch = safe_emalloc(end, sizeof(zend_try_catch_element), 0);
	
	memcpy(
		try_catch, 
		old,
		sizeof(zend_try_catch_element) * end);
	
	return try_catch;
} /* }}} */

static inline zend_live_range* uopz_copy_live(zend_live_range *old, int end) { /* {{{ */
	zend_live_range *range = safe_emalloc(end, sizeof(zend_live_range), 0);

	memcpy(
		range,
		old,
		sizeof(zend_live_range) * end);

	return range;
} /* }}} */

/* {{{ */
static inline zval* uopz_copy_literals(zval *old, int end) {
	zval *literals = (zval*) safe_emalloc(end, sizeof(zval), 0);
	int it = 0;

	memcpy(literals, old, sizeof(zval) * end);

	while (it < end) {
		zval_copy_ctor(&literals[it]);	
		it++;
	}
	
	return literals;
} /* }}} */

/* {{{ */
static inline zend_op* uopz_copy_opcodes(zend_op_array *op_array, zval *literals) {
	zend_op *copy = safe_emalloc(
		op_array->last, sizeof(zend_op), 0);

	memcpy(copy, op_array->opcodes, sizeof(zend_op) * op_array->last);

	{
		zend_op *opline = copy;
		zend_op *end    = copy + op_array->last;

		for (; opline < end; opline++) {
#if ZEND_USE_ABS_CONST_ADDR
			if (opline->op1_type == IS_CONST)
				opline->op1.zv = (zval*)((char*)opline->op1.zv + ((char*)op_array->literals - (char*)literals));
			if (opline->op2_type == IS_CONST) 
				opline->op2.zv = (zval*)((char*)opline->op2.zv + ((char*)op_array->literals - (char*)literals));
#elif PHP_VERSION_ID >= 70300
			if (opline->op1_type == IS_CONST) {
				opline->op1.constant =
					(char*)(op_array->literals +
						((zval*)((char*)(op_array->opcodes + (opline - copy)) +
						(int32_t)opline->op1.constant) - literals)) -
					(char*)opline;
				if (opline->opcode == ZEND_SEND_VAL
				 || opline->opcode == ZEND_SEND_VAL_EX
				 || opline->opcode == ZEND_QM_ASSIGN) {
					zend_vm_set_opcode_handler_ex(opline, 0, 0, 0);
				}
			}
			if (opline->op2_type == IS_CONST) {
				opline->op2.constant =
					(char*)(op_array->literals +
						((zval*)((char*)(op_array->opcodes + (opline - copy)) +
						(int32_t)opline->op2.constant) - literals)) -
					(char*)opline;
			}
#endif

#if ZEND_USE_ABS_JMP_ADDR
			if ((op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO) != 0) {
				switch (opline->opcode) {
					case ZEND_JMP:
					case ZEND_FAST_CALL:
					case ZEND_DECLARE_ANON_CLASS:
					case ZEND_DECLARE_ANON_INHERITED_CLASS:
						 opline->op1.jmp_addr = &copy[opline->op1.jmp_addr - op_array->opcodes];
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
						opline->op2.jmp_addr = &copy[opline->op2.jmp_addr - op_array->opcodes];
					break;
				}
			}
#endif
		}
	}

	return copy;
} /* }}} */

/* {{{ */
static inline zend_arg_info* uopz_copy_arginfo(zend_op_array *op_array, zend_arg_info *old, uint32_t end) {
	zend_arg_info *info;
	uint32_t it = 0;

	if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		old--;
		end++;
	}

	if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
		end++;
	}

	info = safe_emalloc
		(end, sizeof(zend_arg_info), 0);
	memcpy(info, old, sizeof(zend_arg_info) * end);	

	while (it < end) {
		if (info[it].name)
			info[it].name = zend_string_copy(old[it].name);
#if PHP_VERSION_ID >= 70200
		if (ZEND_TYPE_IS_SET(old[it].type) && ZEND_TYPE_IS_CLASS(old[it].type)) {
			info[it].type = ZEND_TYPE_ENCODE_CLASS(
				zend_string_copy(
					ZEND_TYPE_NAME(info[it].type)), 
				ZEND_TYPE_ALLOW_NULL(info[it].type));
		}
#else
		if (info[it].class_name)
			info[it].class_name = zend_string_copy(old[it].class_name);
#endif
		it++;
	}
	
	if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		info++;
	}
	
	return info;
} /* }}} */

zend_function* uopz_copy_closure(zend_class_entry *scope, zend_function *function, zend_long flags, zend_function *prototype) { /* {{{ */
	zend_function  *copy;	
	zend_op_array  *op_array;
	zend_string   **variables;
	zval           *literals;
	zend_arg_info  *arg_info;

	copy = (zend_function*) 
		zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
	memcpy(copy, function, sizeof(zend_op_array));
	
	op_array = &copy->op_array;
	variables = op_array->vars;
	literals = op_array->literals;
	arg_info = op_array->arg_info;

	op_array->function_name = zend_string_dup(op_array->function_name, 0);
	op_array->refcount = emalloc(sizeof(uint32_t));
	(*op_array->refcount) = 1;

	op_array->fn_flags &= ~ ZEND_ACC_CLOSURE | ZEND_ACC_PPP_MASK;	
	op_array->fn_flags |= ZEND_ACC_ARENA_ALLOCATED;
	op_array->fn_flags |= flags;
	op_array->scope = scope;
	op_array->prototype = prototype;

	op_array->run_time_cache = zend_arena_alloc(&CG(arena), op_array->cache_size);

	if (op_array->doc_comment) {
		op_array->doc_comment = zend_string_copy(op_array->doc_comment);
	}

	if (op_array->literals) {
		op_array->literals = uopz_copy_literals (literals, op_array->last_literal);
	}

	op_array->opcodes = uopz_copy_opcodes(op_array, literals);

	if (op_array->arg_info) {
		op_array->arg_info = uopz_copy_arginfo(op_array, arg_info, op_array->num_args);
	}

	if (op_array->live_range) {
		op_array->live_range = uopz_copy_live(op_array->live_range, op_array->last_live_range);
	}

	if (op_array->try_catch_array) {
		op_array->try_catch_array = uopz_copy_try(op_array->try_catch_array, op_array->last_try_catch);
	}

	if (op_array->vars) {
		op_array->vars = uopz_copy_variables(variables, op_array->last_var);
	}

	if (op_array->static_variables) {
		op_array->static_variables = uopz_copy_statics(op_array->static_variables);
	}

	return copy;
} /* }}} */
#endif


