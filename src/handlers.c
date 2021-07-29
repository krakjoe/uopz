/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2016-2021                                  |
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

#define UOPZ_HANDLERS_COUNT 12

#ifdef ZEND_VM_FP_GLOBAL_REG
#	define UOPZ_OPCODE_HANDLER_ARGS
#	define UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU
#	define UOPZ_OPCODE_HANDLER_ARGS_CC
#	define UOPZ_OPCODE_HANDLER_ARGS_DC
#else
#	define UOPZ_OPCODE_HANDLER_ARGS zend_execute_data *execute_data
#	define UOPZ_OPCODE_HANDLER_ARGS_DC , zend_execute_data *execute_data
#	define UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU execute_data
#	define UOPZ_OPCODE_HANDLER_ARGS_CC , execute_data
#endif

#ifdef ZEND_VM_IP_GLOBAL_REG
#	define UOPZ_OPLINE opline
#	define UOPZ_USE_OPLINE
#	define UOPZ_LOAD_OPLINE() opline = EX(opline)
#	define UOPZ_LOAD_NEXT_OPLINE() opline = EX(opline) + 1
#	define UOPZ_SAVE_OPLINE() EX(opline) = opline
#else
#	define UOPZ_OPLINE EX(opline)
#	define UOPZ_USE_OPLINE const zend_op *opline = EX(opline)
#	define UOPZ_LOAD_OPLINE()
#	define UOPZ_LOAD_NEXT_OPLINE() UOPZ_OPLINE++
#	define UOPZ_SAVE_OPLINE()
#endif

#define UOPZ_HANDLE_EXCEPTION() UOPZ_LOAD_OPLINE() return ZEND_USER_OPCODE_CONTINUE

#define UOPZ_VM_DISPATCH() return _uopz_vm_dispatch(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU)
#define UOPZ_VM_RETURN()   return ZEND_USER_OPCODE_RETURN
#define UOPZ_VM_CONTINUE() return ZEND_USER_OPCODE_CONTINUE
#define UOPZ_VM_NEXT(e, n)	do { \
	if (e) { \
		UOPZ_OPLINE = EX(opline) + (n); \
	} else { \
		UOPZ_OPLINE = opline + (n); \
	} \
	\
	UOPZ_VM_CONTINUE(); \
} while(0)
#define UOPZ_VM_LEAVE()	return ZEND_USER_OPCODE_LEAVE

#define RETURN_VALUE_USED(opline) ((opline)->result_type != IS_UNUSED)

#define EX_CONSTANT(e) RT_CONSTANT(EX(opline), e)

#define UOPZ_HANDLERS_DECL_BEGIN() uopz_vm_handler_t uopz_vm_handlers[UOPZ_HANDLERS_COUNT] = {
#define UOPZ_HANDLER_DECL(o, n) 	{o, &zend_vm_##n, uopz_vm_##n},
#define UOPZ_HANDLERS_DECL_END()   {0}};

#define UOPZ_HANDLER_OVERLOAD(h) do { \
	*(h)->zend = zend_get_user_opcode_handler((h)->opcode); \
	zend_set_user_opcode_handler((h)->opcode, (h)->uopz); \
} while (0)

#define UOPZ_HANDLER_RESTORE(h) do { \
	zend_set_user_opcode_handler((h)->opcode, *(h)->zend); \
} while (0)

typedef int (*zend_vm_handler_t) (UOPZ_OPCODE_HANDLER_ARGS);

typedef struct _uopz_vm_handler_t {
	zend_uchar		opcode;
	zend_vm_handler_t *zend;
	zend_vm_handler_t uopz;
} uopz_vm_handler_t;

zend_vm_handler_t zend_vm_exit;
zend_vm_handler_t zend_vm_new;
zend_vm_handler_t zend_vm_fetch_constant;
zend_vm_handler_t zend_vm_do_fcall;
zend_vm_handler_t zend_vm_do_ucall;
zend_vm_handler_t zend_vm_fetch_class;
zend_vm_handler_t zend_vm_fetch_class_constant;
zend_vm_handler_t zend_vm_init_fcall;
zend_vm_handler_t zend_vm_init_fcall_by_name;
zend_vm_handler_t zend_vm_init_ns_fcall_by_name;
zend_vm_handler_t zend_vm_init_method_call;
zend_vm_handler_t zend_vm_init_static_method_call;

int uopz_vm_exit(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_new(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_constant(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_do_fcall(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_do_ucall(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_class_constant(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_fcall(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_ns_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS);

UOPZ_HANDLERS_DECL_BEGIN()
	UOPZ_HANDLER_DECL(ZEND_EXIT,					exit)
	UOPZ_HANDLER_DECL(ZEND_NEW,					 	new)
	UOPZ_HANDLER_DECL(ZEND_FETCH_CONSTANT,		  	fetch_constant)
	UOPZ_HANDLER_DECL(ZEND_FETCH_CLASS_CONSTANT,	fetch_class_constant)
	UOPZ_HANDLER_DECL(ZEND_DO_FCALL,				do_fcall)
	UOPZ_HANDLER_DECL(ZEND_DO_UCALL,				do_ucall)
	UOPZ_HANDLER_DECL(ZEND_INIT_FCALL,			  	init_fcall)
	UOPZ_HANDLER_DECL(ZEND_INIT_FCALL_BY_NAME,	  	init_fcall_by_name)
	UOPZ_HANDLER_DECL(ZEND_INIT_NS_FCALL_BY_NAME,   init_ns_fcall_by_name)
	UOPZ_HANDLER_DECL(ZEND_INIT_METHOD_CALL,		init_method_call)
	UOPZ_HANDLER_DECL(ZEND_INIT_STATIC_METHOD_CALL, init_static_method_call)
UOPZ_HANDLERS_DECL_END()

static zend_always_inline zval* uopz_get_zval(const zend_op *opline, int op_type, const znode_op *node, const zend_execute_data *execute_data) {
	return zend_get_zval_ptr(opline, op_type, node, execute_data);
}

void uopz_handlers_init(void) {
	uopz_vm_handler_t *handler = uopz_vm_handlers;

	while (handler) {
		if (!handler->opcode) {
			break;
		}
		UOPZ_HANDLER_OVERLOAD(handler);
		handler++;
	}
}

void uopz_handlers_shutdown(void) {
	uopz_vm_handler_t *handler = uopz_vm_handlers;
	
	while (handler) {
		if (!handler->opcode) {
			break;
		}
		UOPZ_HANDLER_RESTORE(handler);
		handler++;
	}
}

static zend_always_inline int _uopz_vm_dispatch(UOPZ_OPCODE_HANDLER_ARGS) {
	zend_vm_handler_t zend = NULL;

	switch (EX(opline)->opcode) {
		case ZEND_EXIT:
			zend = zend_vm_exit;
		break;

		case ZEND_NEW:
			zend = zend_vm_new;
		break;

		case ZEND_INIT_FCALL_BY_NAME:
			zend = zend_vm_init_fcall_by_name;
		break;

		case ZEND_INIT_FCALL:
			zend = zend_vm_init_fcall;
		break;

		case ZEND_INIT_NS_FCALL_BY_NAME:
			zend = zend_vm_init_ns_fcall_by_name;
		break;

		case ZEND_INIT_METHOD_CALL:
			zend = zend_vm_init_method_call;
		break;

		case ZEND_INIT_STATIC_METHOD_CALL:
			zend = zend_vm_init_static_method_call;
		break;		

		case ZEND_FETCH_CONSTANT:
			zend = zend_vm_fetch_constant;
		break;

		case ZEND_FETCH_CLASS_CONSTANT:
			zend = zend_vm_fetch_class_constant;
		break;

		case ZEND_DO_FCALL:
			zend = zend_vm_do_fcall;
		break;

		case ZEND_DO_UCALL:
			zend = zend_vm_do_ucall;
		break;
	}

	if (zend) {
		return zend(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
}

int uopz_vm_exit(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *estatus;

	if (UOPZ(exit)) {
		UOPZ_VM_DISPATCH();
	}

	if (opline->op1_type != IS_UNUSED) {
		estatus = uopz_get_zval(
				opline,
				opline->op1_type,
				&opline->op1,
				execute_data);

		do {
			if (Z_TYPE_P(estatus) == IS_LONG) {
				EG(exit_status) = Z_LVAL_P(estatus);
			} else {
				if (opline->op1_type & (IS_VAR|IS_CV) && Z_ISREF_P(estatus)) {
					estatus = Z_REFVAL_P(estatus);

					if (Z_TYPE_P(estatus) == IS_LONG) {
						EG(exit_status) = Z_LVAL_P(estatus);
						break;
					}
				}
			}
		} while (0);

		ZVAL_COPY(&UOPZ(estatus), estatus);
	}

	if (opline < &EX(func)->op_array.opcodes[EX(func)->op_array.last - 1]) {
		UOPZ_LOAD_NEXT_OPLINE();

		while (opline->opcode == ZEND_EXT_STMT) {
			UOPZ_LOAD_NEXT_OPLINE();
		}

		UOPZ_VM_CONTINUE();
	} else {
		UOPZ_VM_RETURN();
	}
} /* }}} */

int uopz_vm_new(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *result;
	zend_function *constructor;
	zend_class_entry *ce;
	zend_execute_data *call;
	zend_object *obj = NULL;
	
	UOPZ_SAVE_OPLINE();

	if (opline->op1_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(opline->op1)), &obj, &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(EX_CONSTANT(opline->op1)),
				Z_STR_P(EX_CONSTANT(opline->op1) + 1),
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);

			if (ce == NULL) {
				ZVAL_UNDEF(EX_VAR(opline->result.var));

				UOPZ_VM_DISPATCH();
			}
		}
	} else if (opline->op1_type == IS_UNUSED) {
		ce = zend_fetch_class(
			NULL, opline->op1.num);
		uopz_find_mock(ce->name, &obj, &ce);
	} else {
		ce = Z_CE_P(
			EX_VAR(opline->op1.var));
		uopz_find_mock(ce->name, &obj, &ce);
	}

	if (obj != NULL) {
		ZVAL_OBJ(
			EX_VAR(opline->result.var), obj);
		Z_ADDREF_P(EX_VAR(opline->result.var));

		if (opline->extended_value == 0 && (opline+1)->opcode == ZEND_DO_FCALL) {
			UOPZ_VM_NEXT(0, 2);
		}

		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION, (zend_function *) &zend_pass_function,
			opline->extended_value, NULL
		);

		call->prev_execute_data = EX(call);
		EX(call) = call;

		UOPZ_VM_NEXT(0, 1);
	}

	result = EX_VAR(opline->result.var);

	if (object_init_ex(result, ce) != SUCCESS) {
		ZVAL_UNDEF(result);

		UOPZ_HANDLE_EXCEPTION();
	}

	constructor = Z_OBJ_HT_P(result)->get_constructor(Z_OBJ_P(result));

	if (!constructor) {
		if (EG(exception)) {
			UOPZ_HANDLE_EXCEPTION();
		}

		if (opline->extended_value == 0 && (opline+1)->opcode == ZEND_DO_FCALL) {
			UOPZ_VM_NEXT(0, 2);
		}

		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION, (zend_function *) &zend_pass_function,
			opline->extended_value, NULL);
	} else {
		if (constructor->type == ZEND_USER_FUNCTION && !RUN_TIME_CACHE(&constructor->op_array)) {
			void **run_time_cache =
				zend_arena_alloc(&CG(arena), constructor->op_array.cache_size);
			memset(run_time_cache, 0, constructor->op_array.cache_size);

			ZEND_MAP_PTR_SET(constructor->op_array.run_time_cache, run_time_cache);
		}

		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION | ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS,
			constructor,
			opline->extended_value,
			Z_OBJ_P(result)
		);

		Z_ADDREF_P(result);
	}

	call->prev_execute_data = EX(call);
	EX(call) = call;

	UOPZ_VM_NEXT(0, 1);
} /* }}} */

static zend_always_inline bool uopz_run_hook(zend_function *function, zend_execute_data *execute_data) { /* {{{ */
	uopz_hook_t *uhook = uopz_find_hook(function);

	if (uhook && !uhook->busy) {
		uopz_execute_hook(uhook, execute_data, 0, 0);
		if (UNEXPECTED(EG(exception))) {
			return false;
		}
	}
	return true;
} /* }}} */

/* {{{ */
static zend_always_inline int php_uopz_leave_helper(zend_execute_data *execute_data) {
	zend_execute_data *call = EX(call);

	if (ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS) {
		OBJ_RELEASE(Z_OBJ(call->This));
	} else if (ZEND_CALL_INFO(call) & ZEND_CALL_CLOSURE) {
		OBJ_RELEASE(ZEND_CLOSURE_OBJECT(call->func));
	}

	EX(call) = call->prev_execute_data;
	EX(opline) = EX(opline) + 1;

	zend_vm_stack_free_call_frame(call);

	UOPZ_VM_LEAVE();
} /* }}} */

int uopz_vm_do_call_common(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zend_execute_data *call = EX(call);

	if (call) {
		uopz_return_t *ureturn;

		if (!uopz_run_hook(call->func, call)) {
			return php_uopz_leave_helper(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		}

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
	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_do_ucall(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	return uopz_vm_do_call_common(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
} /* }}} */

int uopz_vm_do_fcall(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	return uopz_vm_do_call_common(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
} /* }}} */

int uopz_vm_call_common(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	CACHE_PTR(EX(opline)->result.num, NULL);

	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_init_fcall(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	return uopz_vm_call_common(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
} /* }}} */

int uopz_vm_init_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	return uopz_vm_call_common(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
} /* }}} */

int uopz_vm_init_ns_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	return uopz_vm_call_common(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
} /* }}} */

int uopz_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	if (EX(opline)->op2_type == IS_CONST) {
		CACHE_PTR(EX(opline)->result.num, NULL);
		CACHE_PTR(EX(opline)->result.num + sizeof(void*), NULL);
	}
	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	if (EX(opline)->op2_type == IS_CONST) {
		if (EX(opline)->op1_type == IS_CONST) {
			CACHE_PTR(EX(opline)->result.num + sizeof(void*), NULL);
		} else {
			CACHE_PTR(EX(opline)->result.num, NULL);
			CACHE_PTR(EX(opline)->result.num + sizeof(void*), NULL);
		}
	}
	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_fetch_constant(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	CACHE_PTR(EX(opline)->extended_value, NULL);
	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_fetch_class_constant(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	CACHE_PTR(EX(opline)->extended_value + sizeof(void*), NULL);
	if (EX(opline)->op1_type != IS_CONST) {
		CACHE_PTR(EX(opline)->extended_value, NULL);
	}
	UOPZ_VM_DISPATCH();
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
