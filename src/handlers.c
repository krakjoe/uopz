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

#ifndef UOPZ_HANDLERS
#define UOPZ_HANDLERS

#include "php.h"
#include "uopz.h"

#include "class.h"
#include "return.h"
#include "hook.h"
#include "util.h"

ZEND_EXTERN_MODULE_GLOBALS(uopz);

#ifdef ZEND_VM_FP_GLOBAL_REG
#	define UOPZ_OPCODE_HANDLER_ARGS
#	define UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU
#else
#	define UOPZ_OPCODE_HANDLER_ARGS zend_execute_data *execute_data
#	define UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU execute_data
#endif

#ifndef GC_ADDREF
#define GC_ADDREF(g) ++GC_REFCOUNT(g)
#endif

#define RETURN_VALUE_USED(opline) ((opline)->result_type != IS_UNUSED)

#if PHP_VERSION_ID >= 70300
#	define EX_CONSTANT(e) RT_CONSTANT(EX(opline), e)
#endif

#define UOPZ_SET_HANDLER(h, o, n) do { \
	(h) = zend_get_user_opcode_handler((o)); \
	zend_set_user_opcode_handler((o), (n)); \
} while (0)

#define UOPZ_UNSET_HANDLER(h, o) do { \
	zend_set_user_opcode_handler(o, h); \
} while (0)

typedef int (*zend_vm_handler_t) (UOPZ_OPCODE_HANDLER_ARGS);

zend_vm_handler_t zend_vm_exit;
zend_vm_handler_t zend_vm_init_fcall;
zend_vm_handler_t zend_vm_init_fcall_by_name;
zend_vm_handler_t zend_vm_init_ns_fcall_by_name;
zend_vm_handler_t zend_vm_init_method_call;
zend_vm_handler_t zend_vm_init_static_method_call;
zend_vm_handler_t zend_vm_new;
zend_vm_handler_t zend_vm_fetch_constant;
zend_vm_handler_t zend_vm_do_fcall;
zend_vm_handler_t zend_vm_fetch_class;
zend_vm_handler_t zend_vm_add_trait;
zend_vm_handler_t zend_vm_add_interface;
zend_vm_handler_t zend_vm_fetch_class_constant;

int uopz_vm_exit(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_fcall(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_ns_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_new(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_constant(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_class(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_do_fcall(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_class_constant(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_add_trait(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_add_interface(UOPZ_OPCODE_HANDLER_ARGS);

static zend_always_inline zval* uopz_get_zval(const zend_op *opline, int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type) {
#if PHP_VERSION_ID >= 70300
	return zend_get_zval_ptr(opline, op_type, node, execute_data, should_free, type);
#else
	return zend_get_zval_ptr(op_type, node, execute_data, should_free, type);
#endif
}

void uopz_handlers_init(void) {
	UOPZ_SET_HANDLER(zend_vm_exit, ZEND_EXIT, uopz_vm_exit);
	UOPZ_SET_HANDLER(zend_vm_init_fcall, ZEND_INIT_FCALL, uopz_vm_init_fcall);
	UOPZ_SET_HANDLER(zend_vm_init_fcall_by_name, ZEND_INIT_FCALL_BY_NAME, uopz_vm_init_fcall_by_name);
	UOPZ_SET_HANDLER(zend_vm_init_ns_fcall_by_name, ZEND_INIT_NS_FCALL_BY_NAME, uopz_vm_init_ns_fcall_by_name);
	UOPZ_SET_HANDLER(zend_vm_init_method_call, ZEND_INIT_METHOD_CALL, uopz_vm_init_method_call);
	UOPZ_SET_HANDLER(zend_vm_init_static_method_call, ZEND_INIT_STATIC_METHOD_CALL, uopz_vm_init_static_method_call);
	UOPZ_SET_HANDLER(zend_vm_new, ZEND_NEW, uopz_vm_new);
	UOPZ_SET_HANDLER(zend_vm_fetch_constant, ZEND_FETCH_CONSTANT, uopz_vm_fetch_constant);
	UOPZ_SET_HANDLER(zend_vm_do_fcall, ZEND_DO_FCALL, uopz_vm_do_fcall);
	UOPZ_SET_HANDLER(zend_vm_fetch_class_constant, ZEND_FETCH_CLASS_CONSTANT, uopz_vm_fetch_class_constant);
	UOPZ_SET_HANDLER(zend_vm_fetch_class, ZEND_FETCH_CLASS, uopz_vm_fetch_class);
	UOPZ_SET_HANDLER(zend_vm_add_trait, ZEND_ADD_TRAIT, uopz_vm_add_trait);
	UOPZ_SET_HANDLER(zend_vm_add_interface, ZEND_ADD_INTERFACE, uopz_vm_add_interface);
}

void uopz_handlers_shutdown(void) {
	UOPZ_UNSET_HANDLER(zend_vm_exit,					ZEND_EXIT);
	UOPZ_UNSET_HANDLER(zend_vm_init_fcall_by_name,		ZEND_INIT_FCALL_BY_NAME);
	UOPZ_UNSET_HANDLER(zend_vm_init_fcall,				ZEND_INIT_FCALL);
	UOPZ_UNSET_HANDLER(zend_vm_init_ns_fcall_by_name,	ZEND_INIT_NS_FCALL_BY_NAME);
	UOPZ_UNSET_HANDLER(zend_vm_init_method_call,		ZEND_INIT_METHOD_CALL);
	UOPZ_UNSET_HANDLER(zend_vm_init_static_method_call,ZEND_INIT_STATIC_METHOD_CALL);
	UOPZ_UNSET_HANDLER(zend_vm_new,					ZEND_NEW);
	UOPZ_UNSET_HANDLER(zend_vm_fetch_constant,			ZEND_FETCH_CONSTANT);
	UOPZ_UNSET_HANDLER(zend_vm_do_fcall,				ZEND_DO_FCALL);
	UOPZ_UNSET_HANDLER(zend_vm_fetch_class_constant,	ZEND_FETCH_CLASS_CONSTANT);
	UOPZ_UNSET_HANDLER(zend_vm_fetch_class,			ZEND_FETCH_CLASS);
	UOPZ_UNSET_HANDLER(zend_vm_add_trait,				ZEND_ADD_TRAIT);
	UOPZ_UNSET_HANDLER(zend_vm_add_interface,			ZEND_ADD_INTERFACE);
}

int uopz_vm_exit(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	if (UOPZ(exit)) {
		if (zend_vm_exit)
			return zend_vm_exit(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

		return ZEND_USER_OPCODE_DISPATCH;
	}

	if (EX(opline)->op1_type != IS_UNUSED) {
		zval *estatus;

		if (EX(opline)->op1_type == IS_CONST) {
			estatus = EX_CONSTANT(EX(opline)->op1);
		} else estatus = EX_VAR(EX(opline)->op1.var);

		if (Z_ISREF_P(estatus)) {
			estatus = Z_REFVAL_P(estatus);
		}

		if (Z_TYPE_P(estatus) == IS_LONG) {
			EG(exit_status) = Z_LVAL_P(estatus);
		} else EG(exit_status) = 0;

		ZVAL_COPY(&UOPZ(estatus), estatus);
	}

	if (EX(opline) < &EX(func)->op_array.opcodes[EX(func)->op_array.last - 1]) {
		EX(opline)++;

		while (EX(opline)->opcode == ZEND_EXT_STMT) {
			EX(opline)++;
		}

		return ZEND_USER_OPCODE_CONTINUE;
	} else {
		return ZEND_USER_OPCODE_RETURN;
	}
} /* }}} */

int uopz_vm_init_fcall(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *name;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	name = uopz_get_zval(
		EX(opline),
		EX(opline)->op2_type,
		&EX(opline)->op2,
		execute_data,
		&free_op2, BP_VAR_R);

	fbc = (zend_function*) zend_hash_find_ptr(EG(function_table), Z_STR_P(name));

	if (!fbc) {
		return ZEND_USER_OPCODE_DISPATCH;	
	}

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		/* TODO immutable for 7.3 */
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame_ex(
		EX(opline)->op1.num, 
		ZEND_CALL_NESTED_FUNCTION, fbc, EX(opline)->extended_value, NULL, NULL);
	call->prev_execute_data = EX(call);
	EX(call) = call;
	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_init_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *name;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	name = uopz_get_zval(
		EX(opline),
		EX(opline)->op2_type,
		&EX(opline)->op2,
		execute_data,
		&free_op2, BP_VAR_R);

	fbc = (zend_function*) zend_hash_find_ptr(EG(function_table), Z_STR_P(name+1));

	if (!fbc) {
		return ZEND_USER_OPCODE_DISPATCH;	
	}

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		/* TODO immutable for 7.3 */
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame(
		ZEND_CALL_NESTED_FUNCTION, 
		fbc, EX(opline)->extended_value, NULL, NULL);
	call->prev_execute_data = EX(call);
	EX(call) = call;
	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_init_ns_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *name;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	name = uopz_get_zval(
		EX(opline),
		EX(opline)->op2_type,
		&EX(opline)->op2,
		execute_data,
		&free_op2, BP_VAR_R) + 1;

	fbc = (zend_function*) zend_hash_find_ptr(EG(function_table), Z_STR_P(name));

	if (!fbc) {
		name++;
		fbc = zend_hash_find_ptr(EG(function_table), Z_STR_P(name));
		if (!fbc) {
			zend_throw_error(NULL,
				"Call to undefined function %s()",
				Z_STRVAL_P(EX_CONSTANT(EX(opline)->op2)));
			return ZEND_USER_OPCODE_CONTINUE;
		}
	}

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		/* TODO immutable for 7.3 */
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame(
		ZEND_CALL_NESTED_FUNCTION, 
		fbc, EX(opline)->extended_value, NULL, NULL);
	call->prev_execute_data = EX(call);
	EX(call) = call;
	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zend_string *clazz;
	zend_object *object;
	zval *method;
	zend_class_entry *ce;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op1;
	zend_free_op free_op2;

	if (EX(opline)->op1_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(EX(opline)->op1)), &ce) != SUCCESS) {
			if (zend_vm_init_static_method_call)
				return zend_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			return ZEND_USER_OPCODE_DISPATCH;
		}
	} else if (EX(opline)->op1_type == IS_UNUSED) {
		ce = zend_fetch_class(NULL, EX(opline)->op1.num);

		if (uopz_find_mock(ce->name, &ce) != SUCCESS) {
			if (zend_vm_init_static_method_call)
				return zend_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			return ZEND_USER_OPCODE_DISPATCH;
		}
	} else {
		ce = Z_CE_P(EX_VAR(EX(opline)->op1.var));

		if (uopz_find_mock(ce->name, &ce) != SUCCESS) {
			if (zend_vm_init_static_method_call)
				return zend_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			return ZEND_USER_OPCODE_DISPATCH;
		}
	}

	if (EX(opline)->op2_type != IS_UNUSED) {
		method = uopz_get_zval(
			EX(opline),
			EX(opline)->op2_type,
			&EX(opline)->op2,
			execute_data,
			&free_op2, BP_VAR_R);

		if (EX(opline)->op2_type != IS_CONST) {
			if (Z_TYPE_P(method) != IS_STRING) {
				do {
					if (EX(opline)->op2_type & (IS_VAR|IS_CV) && Z_ISREF_P(method)) {
						method = Z_REFVAL_P(method);

						if (Z_TYPE_P(method) == IS_STRING) {
							break;
						}
					}

					if (zend_vm_init_static_method_call)
						return zend_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

					return ZEND_USER_OPCODE_DISPATCH;
				} while (0);
			}
		}

		if (ce->get_static_method) {
			fbc = ce->get_static_method(ce, Z_STR_P(method));
		} else {
			fbc = zend_std_get_static_method(ce, 
				Z_STR_P(method), 
				((EX(opline)->op2_type == IS_CONST) ? 
					(EX_CONSTANT(EX(opline)->op2) + 1) : NULL));
		}

		if (fbc == NULL) {
			if (zend_vm_init_static_method_call)
				return zend_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			return ZEND_USER_OPCODE_DISPATCH;
		}

		if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
			fbc->op_array.run_time_cache = 
				zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
			memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
		}

		if (EX(opline)->op2_type != IS_CONST) {
			if (free_op2) {
				zval_ptr_dtor(free_op2);
			}
		}
	} else {
		if (ce->constructor == NULL) {
			zend_throw_error(NULL, 
				"Cannot call constructor");
			return ZEND_USER_OPCODE_CONTINUE;
		}

		if (Z_TYPE(EX(This)) == IS_OBJECT && Z_OBJ(EX(This))->ce != ce->constructor->common.scope && (ce->constructor->common.fn_flags & ZEND_ACC_PRIVATE)) {
			zend_throw_error(NULL, 
				"Cannot call private %s::__construct()", ZSTR_VAL(ce->name));
			return ZEND_USER_OPCODE_CONTINUE;
		}

		fbc = ce->constructor;

		if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
			fbc->op_array.run_time_cache = 
				zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
			memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
		}
	}

	object = NULL;
	
	if (!(fbc->common.fn_flags & ZEND_ACC_STATIC)) {
		if (Z_TYPE(EX(This)) == IS_OBJECT && instanceof_function(Z_OBJCE(EX(This)), ce)) {
			object = Z_OBJ(EX(This));
			ce = object->ce;
		} else {
			if (fbc->common.fn_flags & ZEND_ACC_ALLOW_STATIC) {
				zend_error(
					E_DEPRECATED,
					"Non-static method %s::%s() should not be called statically",
					ZSTR_VAL(fbc->common.scope->name), ZSTR_VAL(fbc->common.function_name));
			} else {
				zend_throw_error(
					zend_ce_error,
					"Non-static method %s::%s() cannot be called statically",
					ZSTR_VAL(fbc->common.scope->name), ZSTR_VAL(fbc->common.function_name));
			}

			if (EG(exception)) {
				return ZEND_USER_OPCODE_CONTINUE;
			}
		}
	}

	if (EX(opline)->op1_type == IS_UNUSED) {
		if ((EX(opline)->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_PARENT ||
		    (EX(opline)->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_SELF) {
			if (Z_TYPE(EX(This)) == IS_OBJECT) {
				ce = Z_OBJCE(EX(This));
			} else {
				ce = Z_CE(EX(This));
			}
			/* find mock and method */
		}
	}

	call = zend_vm_stack_push_call_frame(
		ZEND_CALL_NESTED_FUNCTION, 
		fbc, EX(opline)->extended_value, ce, object);

	call->prev_execute_data = EX(call);
	EX(call) = call;
	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *object;
	zend_object *obj, *oobj;
	zend_function *fbc;
	zend_class_entry *scope;
	zend_execute_data *call;
	zval *method;
	zend_class_entry *mock;
	uint32_t info;
	zend_free_op free_op1;
	zend_free_op free_op2;

	object = uopz_get_zval(
			EX(opline),
			EX(opline)->op1_type,
			&EX(opline)->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (!object || Z_TYPE_P(object) == IS_UNDEF) {
		if (zend_vm_init_method_call)
			return zend_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

		return ZEND_USER_OPCODE_DISPATCH;
	}

	method = uopz_get_zval(
			EX(opline),
			EX(opline)->op2_type,
			&EX(opline)->op2,
			execute_data,
			&free_op2, BP_VAR_R);

	if (!method) {
		if (zend_vm_init_method_call)
			return zend_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

		return ZEND_USER_OPCODE_DISPATCH;
	}

	if (Z_TYPE_P(method) != IS_STRING) {
		do {
			if ((EX(opline)->op2_type & (IS_VAR|IS_CV)) && Z_ISREF_P(method)) {
				method = Z_REFVAL_P(method);

				if (Z_TYPE_P(method) == IS_STRING) {
					break;
				}
			}
			
			if (zend_vm_init_method_call)
				return zend_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			return ZEND_USER_OPCODE_DISPATCH;
		} while(0);
	}

	if (EX(opline)->op1_type != IS_UNUSED && Z_TYPE_P(object) != IS_OBJECT) {
		do {
			if (EX(opline)->op1_type == IS_CONST || Z_TYPE_P(object) != IS_OBJECT) {
				if ((EX(opline)->op1_type & (IS_VAR|IS_CV)) && Z_ISREF_P(object)) {
					object = Z_REFVAL_P(object);

					if (Z_TYPE_P(object) == IS_OBJECT) {
						break;
					}
				
				}
			}

			if (zend_vm_init_method_call)
				return zend_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			return ZEND_USER_OPCODE_DISPATCH;
		} while (0);
	}

	oobj = obj = Z_OBJ_P(object);
	scope = obj->ce;

	if (obj->handlers->get_method == NULL) {
		if (zend_vm_init_method_call)
			return zend_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

		return ZEND_USER_OPCODE_DISPATCH;
	}

	fbc = obj->handlers->get_method(&obj, 
			Z_STR_P(method), 
			((EX(opline)->op2_type == IS_CONST) ? 
				(EX_CONSTANT(EX(opline)->op2) + 1) : NULL));

	if (!fbc) {
		if (zend_vm_init_method_call)
			return zend_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

		return ZEND_USER_OPCODE_DISPATCH;
	}

	if (EX(opline)->op2_type != IS_CONST) {
		if (free_op2) {
			zval_ptr_dtor(free_op2);
		}
	}

	if (uopz_find_mock(scope->name, &mock) == SUCCESS) {
		uopz_find_method(
			mock, Z_STR_P(method), &fbc);
	}

	info = ZEND_CALL_NESTED_FUNCTION;

	if ((fbc->common.fn_flags & ZEND_ACC_STATIC) != 0) {
		obj = NULL;
#if PHP_VERSION_ID >= 70300
		if (free_op1) {
			zval_ptr_dtor(free_op1);
		}

		if ((EX(opline)->op1_type & (IS_VAR|IS_TMP_VAR)) && EG(exception)) {
			return ZEND_USER_OPCODE_CONTINUE;
		}
#endif
	} else if (EX(opline)->op1_type & (IS_VAR|IS_TMP_VAR|IS_CV)) {
		info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_RELEASE_THIS;

#if PHP_VERSION_ID >= 70300
		if (EX(opline)->op1_type == IS_CV) {
			GC_ADDREF(Z_OBJ_P(object));
		} else if (free_op1 != object) {
			GC_ADDREF(Z_OBJ_P(object));
			zval_ptr_dtor(free_op1);
		}
#else
		GC_ADDREF(Z_OBJ_P(object));
#endif
	}

#if PHP_VERSION_ID < 70300
	if (free_op1) {
		zval_ptr_dtor(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor(free_op2);
	}

	if ((EX(opline)->op1_type & (IS_VAR|IS_TMP_VAR)) && EG(exception)) {
		return ZEND_USER_OPCODE_CONTINUE;
	}
#endif

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame(info, 
		fbc, EX(opline)->extended_value, scope, obj);

	call->prev_execute_data = EX(call);
	EX(call) = call;
	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_new(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *result;
	zend_function *constructor;
	zend_class_entry *ce;
	zend_execute_data *call;

	if (EX(opline)->op1_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(EX(opline)->op1)), &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(EX_CONSTANT(EX(opline)->op1)), 
				EX_CONSTANT(EX(opline)->op1) + 1, 
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);

			if (ce == NULL) {
				ZVAL_UNDEF(EX_VAR(EX(opline)->result.var));

				return ZEND_USER_OPCODE_DISPATCH;
			}
		}
	} else if (EX(opline)->op1_type == IS_UNUSED) {
		ce = zend_fetch_class(
			NULL, EX(opline)->op1.num);
		uopz_find_mock(ce->name, &ce);	
	} else {
		ce = Z_CE_P(
			EX_VAR(EX(opline)->op1.var));
		uopz_find_mock(ce->name, &ce);
	}

	result = EX_VAR(EX(opline)->result.var);

	if (object_init_ex(result, ce) != SUCCESS) {
		ZVAL_UNDEF(result);

		return ZEND_USER_OPCODE_CONTINUE;
	}

	constructor = Z_OBJ_HT_P(result)->get_constructor(Z_OBJ_P(result));

	if (!constructor) {
		if (EG(exception)) {
			return ZEND_USER_OPCODE_CONTINUE;
		}

		if (EX(opline)->extended_value == 0 && (EX(opline)+1)->opcode == ZEND_DO_FCALL) {
			EX(opline) = EX(opline) + 2;

			return ZEND_USER_OPCODE_CONTINUE;
		}

		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION, (zend_function *) &zend_pass_function,
			EX(opline)->extended_value, NULL, NULL);
	} else {
		if (constructor->type == ZEND_USER_FUNCTION && !constructor->op_array.run_time_cache) {
			constructor->op_array.run_time_cache = 
				zend_arena_alloc(&CG(arena), constructor->op_array.cache_size);
			memset(constructor->op_array.run_time_cache, 0, constructor->op_array.cache_size);
		}

		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION | ZEND_CALL_RELEASE_THIS | ZEND_CALL_CTOR,
			constructor,
			EX(opline)->extended_value,
			ce,
			Z_OBJ_P(result));

		Z_ADDREF_P(result);
	}

	call->prev_execute_data = EX(call);
	EX(call) = call;

	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

static zend_always_inline void uopz_run_hook(zend_function *function, zend_execute_data *execute_data) { /* {{{ */
	uopz_hook_t *uhook = uopz_find_hook(function);

	if (uhook && !uhook->busy) {
		uopz_execute_hook(uhook, execute_data);
	}
} /* }}} */

/* {{{ */
static zend_always_inline int php_uopz_leave_helper(zend_execute_data *execute_data) {
	zend_execute_data *call = EX(call);

	EX(call) = call->prev_execute_data;
	EX(opline) = EX(opline) + 1;

	zend_vm_stack_free_call_frame(call);

	return ZEND_USER_OPCODE_LEAVE;
} /* }}} */

int uopz_vm_do_fcall(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zend_execute_data *call = EX(call);

	if (call) {
		uopz_return_t *ureturn;

		uopz_run_hook(call->func, call);

		ureturn = uopz_find_return(call->func);

		if (ureturn) {
			const zend_op *opline = EX(opline);
			zval rv, *return_value = RETURN_VALUE_USED(opline) ? 
				EX_VAR(EX(opline)->result.var) : &rv;

			if (UOPZ_RETURN_IS_EXECUTABLE(ureturn)) {
				if (UOPZ_RETURN_IS_BUSY(ureturn)) {
					goto _uopz_vm_do_fcall_dispatch;
				}

				uopz_execute_return(ureturn, call, return_value);

				if (!RETURN_VALUE_USED(opline)) {
					zval_ptr_dtor(&rv);
				}

				return php_uopz_leave_helper(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
			}

			if (RETURN_VALUE_USED(opline)) {
				ZVAL_COPY(return_value, &ureturn->value);
			}

			return php_uopz_leave_helper(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		}
	}

_uopz_vm_do_fcall_dispatch:
	if (zend_vm_do_fcall) {
		return zend_vm_do_fcall(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

int uopz_vm_fetch_constant(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
#if PHP_VERSION_ID >= 70300
	CACHE_PTR(EX(opline)->extended_value, NULL);
#else
	if (CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)))) {
		CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), NULL);
	}
#endif

	if (zend_vm_fetch_constant) {
		return zend_vm_fetch_constant(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

int uopz_vm_fetch_class_constant(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zend_class_entry *ce, *scope;
	zend_class_constant *c;
	zval *value;

	if (EX(opline)->op1_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(EX(opline)->op1)), &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(EX_CONSTANT(EX(opline)->op1)), 
				EX_CONSTANT(EX(opline)->op1) + 1, 
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);

			if (!ce) {
				ZVAL_UNDEF(EX_VAR(EX(opline)->result.var));

				return ZEND_USER_OPCODE_CONTINUE;
			}
		}
	} else {
		if (EX(opline)->op1_type == IS_UNUSED) {
			ce = zend_fetch_class(NULL, EX(opline)->op1.num);
			if (!ce) {
				ZVAL_UNDEF(EX_VAR(EX(opline)->result.var));

				return ZEND_USER_OPCODE_CONTINUE;
			}
			uopz_find_mock(ce->name, &ce);
		} else {
			ce = 
				Z_CE_P(EX_VAR(EX(opline)->op1.var));
			uopz_find_mock(ce->name, &ce);
		}
	}

	if ((c = zend_hash_find_ptr(&ce->constants_table, Z_STR_P(EX_CONSTANT(EX(opline)->op2))))) {
		scope = EX(func)->op_array.scope;
		if (!zend_verify_const_access(c, scope)) {
			zend_throw_error(NULL, 
				"Cannot access const %s::%s", 
				ZSTR_VAL(ce->name), Z_STRVAL_P(EX_CONSTANT(EX(opline)->op2)));
			ZVAL_UNDEF(EX_VAR(EX(opline)->result.var));

			return ZEND_USER_OPCODE_CONTINUE;
		}

		value = &c->value;
		if (Z_CONSTANT_P(value)) {
			zval_update_constant_ex(value, c->ce);
			if (EG(exception)) {
				ZVAL_UNDEF(EX_VAR(EX(opline)->result.var));

				return ZEND_USER_OPCODE_CONTINUE;
			}
		}
	} else {
		zend_throw_error(NULL, 
			"Undefined class constant '%s'", 
			Z_STRVAL_P(EX_CONSTANT(EX(opline)->op2)));
		ZVAL_UNDEF(EX_VAR(EX(opline)->result.var));

		return ZEND_USER_OPCODE_CONTINUE;
	}

#ifdef ZTS
	if (ce->type == ZEND_INTERNAL_CLASS) {
		ZVAL_DUP(EX_VAR(EX(opline)->result.var), value);
	} else {
		ZVAL_COPY(EX_VAR(EX(opline)->result.var), value);
	}
#else
	ZVAL_COPY(EX_VAR(EX(opline)->result.var), value);
#endif

	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_fetch_class(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *name;
	zend_free_op free_op2;
	
	if (EX(opline)->op2_type == IS_UNUSED) {
		Z_CE_P(EX_VAR(EX(opline)->result.var)) = 
			zend_fetch_class(NULL, EX(opline)->op1.num);
		
		if (!EG(exception)) {
			uopz_find_mock(
				Z_CE_P(EX_VAR(EX(opline)->result.var))->name, 
				&Z_CE_P(EX_VAR(EX(opline)->result.var)));
		}

		EX(opline) = EX(opline) + 1;
		return ZEND_USER_OPCODE_CONTINUE;
	} else if (EX(opline)->op2_type == IS_CONST) {
		name = uopz_get_zval(
			EX(opline),
			EX(opline)->op2_type,
			&EX(opline)->op2,
			execute_data,
			&free_op2, BP_VAR_R);

		if (uopz_find_mock(Z_STR_P(name), &Z_CE_P(EX_VAR(EX(opline)->result.var))) != SUCCESS) {
			Z_CE_P(EX_VAR(EX(opline)->result.var)) = zend_fetch_class_by_name(
									Z_STR_P(name), 
									name + 1, 
									EX(opline)->op1.num);
		}
	} else {
		name = uopz_get_zval(
			EX(opline),
			EX(opline)->op2_type,
			&EX(opline)->op2,
			execute_data,
			&free_op2, BP_VAR_R);
_uopz_vm_fetch_class_try:
		if (Z_TYPE_P(name) == IS_OBJECT) {
			if (uopz_find_mock(Z_OBJCE_P(name)->name, &Z_CE_P(EX_VAR(EX(opline)->result.var))) != SUCCESS) {
				Z_CE_P(EX_VAR(EX(opline)->result.var)) = Z_OBJCE_P(name);	
			}
		} else if (Z_TYPE_P(name) == IS_STRING) {
			if (uopz_find_mock(Z_STR_P(name), &Z_CE_P(EX_VAR(EX(opline)->result.var))) != SUCCESS) {
				Z_CE_P(EX_VAR(EX(opline)->result.var)) = zend_fetch_class(Z_STR_P(name), EX(opline)->op1.num);
			}
		} else if (EX(opline)->op2_type & (IS_VAR|IS_CV) && Z_TYPE_P(name) == IS_REFERENCE) {
			name = Z_REFVAL_P(name);
			goto _uopz_vm_fetch_class_try;
		} else {
			if (EX(opline)->op2_type == IS_CV && Z_TYPE_P(name) == IS_UNDEF) {
				if (EG(exception)) {
					return ZEND_USER_OPCODE_CONTINUE;
				}
			}
			zend_throw_error(NULL, "Class name must be a valid object or a string");
		}
	}

	if (free_op2) {
		zval_ptr_dtor(free_op2);
	}

	EX(opline) = EX(opline) + 1;
	
	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_add_trait(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zend_class_entry *ce = Z_CE_P(EX_VAR(EX(opline)->op1.var)), 
                         *trait;
	zval *name = EX_CONSTANT(EX(opline)->op2);

	if (uopz_find_mock(Z_STR_P(name), &trait) != SUCCESS) {
		trait = zend_fetch_class_by_name(
				Z_STR_P(name), 
				name + 1, 
				ZEND_FETCH_CLASS_TRAIT);

		if (!trait) {
			return ZEND_USER_OPCODE_CONTINUE;
		}
	}

	if (!(trait->ce_flags & ZEND_ACC_TRAIT)) {
		zend_error_noreturn(E_ERROR, "%s cannot use %s - it is not a trait", ZSTR_VAL(ce->name), ZSTR_VAL(trait->name));
	}

	zend_do_implement_trait(ce, trait);

	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

int uopz_vm_add_interface(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zend_class_entry *ce = Z_CE_P(EX_VAR(EX(opline)->op1.var)), 
                         *iface;
	zval *name = EX_CONSTANT(EX(opline)->op2);

	if (uopz_find_mock(Z_STR_P(name), &iface) != SUCCESS) {
		iface = zend_fetch_class_by_name(
				Z_STR_P(name), 
				name + 1, 
				ZEND_FETCH_CLASS_TRAIT);

		if (!iface) {
			return ZEND_USER_OPCODE_CONTINUE;
		}
	}

	if (!(iface->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error_noreturn(E_ERROR, "%s cannot implement %s - it is not an interface", ZSTR_VAL(ce->name), ZSTR_VAL(iface->name));
	}

	zend_do_implement_interface(ce, iface);

	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_CONTINUE;
} /* }}} */

#endif	/* UOPZ_HANDLERS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
