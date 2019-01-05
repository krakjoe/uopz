/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2016-2019                                  |
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

#define UOPZ_HANDLERS_COUNT 30

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

#ifndef GC_ADDREF
#define GC_ADDREF(g) ++GC_REFCOUNT(g)
#endif

#define RETURN_VALUE_USED(opline) ((opline)->result_type != IS_UNUSED)

#if PHP_VERSION_ID >= 70300
#	define EX_CONSTANT(e) RT_CONSTANT(UOPZ_OPLINE, e)
#endif

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
	zend_uchar        opcode;
	zend_vm_handler_t *zend;
	zend_vm_handler_t uopz;
} uopz_vm_handler_t;

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

zend_vm_handler_t zend_vm_fetch_static_prop_r;
zend_vm_handler_t zend_vm_fetch_static_prop_w;
zend_vm_handler_t zend_vm_fetch_static_prop_rw;
zend_vm_handler_t zend_vm_fetch_static_prop_is;
zend_vm_handler_t zend_vm_fetch_static_prop_func_arg;
zend_vm_handler_t zend_vm_fetch_static_prop_unset;
zend_vm_handler_t zend_vm_unset_static_prop;
zend_vm_handler_t zend_vm_isset_isempty_static_prop;

zend_vm_handler_t zend_vm_fetch_obj_r;
zend_vm_handler_t zend_vm_fetch_obj_w;
zend_vm_handler_t zend_vm_fetch_obj_rw;
zend_vm_handler_t zend_vm_fetch_obj_is;
zend_vm_handler_t zend_vm_fetch_obj_func_arg;
zend_vm_handler_t zend_vm_fetch_obj_unset;
zend_vm_handler_t zend_vm_assign_obj;
zend_vm_handler_t zend_vm_isset_isempty_prop_obj;

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

int uopz_vm_fetch_static_prop_r(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_static_prop_w(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_static_prop_rw(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_static_prop_is(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_static_prop_func_arg(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_static_prop_unset(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_unset_static_prop(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_isset_isempty_static_prop(UOPZ_OPCODE_HANDLER_ARGS);

int uopz_vm_fetch_static_helper(int type UOPZ_OPCODE_HANDLER_ARGS_DC);

int uopz_vm_fetch_obj_r(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_obj_w(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_obj_rw(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_obj_is(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_obj_func_arg(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_fetch_obj_unset(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_isset_isempty_prop_obj(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_vm_assign_obj(UOPZ_OPCODE_HANDLER_ARGS);

UOPZ_HANDLERS_DECL_BEGIN()
	UOPZ_HANDLER_DECL(ZEND_EXIT,                    exit)
	UOPZ_HANDLER_DECL(ZEND_INIT_FCALL,              init_fcall)
	UOPZ_HANDLER_DECL(ZEND_INIT_FCALL_BY_NAME,      init_fcall_by_name)
	UOPZ_HANDLER_DECL(ZEND_INIT_NS_FCALL_BY_NAME,   init_ns_fcall_by_name)
	UOPZ_HANDLER_DECL(ZEND_INIT_STATIC_METHOD_CALL, init_static_method_call)
	UOPZ_HANDLER_DECL(ZEND_INIT_METHOD_CALL,        init_method_call)
	UOPZ_HANDLER_DECL(ZEND_NEW,                     new)
	UOPZ_HANDLER_DECL(ZEND_FETCH_CONSTANT,          fetch_constant)
	UOPZ_HANDLER_DECL(ZEND_FETCH_CLASS,             fetch_class)
	UOPZ_HANDLER_DECL(ZEND_FETCH_CLASS_CONSTANT,    fetch_class_constant)
	UOPZ_HANDLER_DECL(ZEND_DO_FCALL,                do_fcall)
	UOPZ_HANDLER_DECL(ZEND_ADD_TRAIT,               add_trait)
	UOPZ_HANDLER_DECL(ZEND_ADD_INTERFACE,           add_interface)
	UOPZ_HANDLER_DECL(ZEND_FETCH_STATIC_PROP_R,        fetch_static_prop_r)
	UOPZ_HANDLER_DECL(ZEND_FETCH_STATIC_PROP_W,        fetch_static_prop_w)
	UOPZ_HANDLER_DECL(ZEND_FETCH_STATIC_PROP_RW,       fetch_static_prop_rw)
	UOPZ_HANDLER_DECL(ZEND_FETCH_STATIC_PROP_IS,       fetch_static_prop_is)
	UOPZ_HANDLER_DECL(ZEND_FETCH_STATIC_PROP_FUNC_ARG, fetch_static_prop_func_arg)
	UOPZ_HANDLER_DECL(ZEND_FETCH_STATIC_PROP_UNSET,    fetch_static_prop_unset)
	UOPZ_HANDLER_DECL(ZEND_UNSET_STATIC_PROP,          unset_static_prop)
	UOPZ_HANDLER_DECL(ZEND_ISSET_ISEMPTY_STATIC_PROP,  isset_isempty_static_prop)
	UOPZ_HANDLER_DECL(ZEND_FETCH_OBJ_R,                fetch_obj_r)
	UOPZ_HANDLER_DECL(ZEND_FETCH_OBJ_W,                fetch_obj_w)
	UOPZ_HANDLER_DECL(ZEND_FETCH_OBJ_RW,               fetch_obj_rw)
	UOPZ_HANDLER_DECL(ZEND_FETCH_OBJ_IS,               fetch_obj_is)
	UOPZ_HANDLER_DECL(ZEND_FETCH_OBJ_FUNC_ARG,         fetch_obj_func_arg)
	UOPZ_HANDLER_DECL(ZEND_FETCH_OBJ_UNSET,            fetch_obj_unset)
	UOPZ_HANDLER_DECL(ZEND_ASSIGN_OBJ,                 assign_obj)
	UOPZ_HANDLER_DECL(ZEND_ISSET_ISEMPTY_PROP_OBJ,     isset_isempty_prop_obj)
UOPZ_HANDLERS_DECL_END()

static zend_always_inline zval* uopz_get_zval(const zend_op *opline, int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type) {
#if PHP_VERSION_ID >= 70300
	return zend_get_zval_ptr(opline, op_type, node, execute_data, should_free, type);
#else
	return zend_get_zval_ptr(op_type, node, execute_data, should_free, type);
#endif
}

#if PHP_VERSION_ID < 70300
static zend_always_inline int uopz_is_by_ref_func_arg_fetch(const zend_op *opline, zend_execute_data *call) {
	uint32_t args = opline->extended_value & ZEND_FETCH_ARG_MASK;

	if (args < MAX_ARG_FLAG_NUM) {
		return QUICK_ARG_SHOULD_BE_SENT_BY_REF(call->func, args);
	}

	return ARG_SHOULD_BE_SENT_BY_REF(call->func, args);
}
#endif

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

		case ZEND_INIT_FCALL:
			zend = zend_vm_init_fcall;
		break;

		case ZEND_INIT_FCALL_BY_NAME:
			zend = zend_vm_init_fcall_by_name;
		break;

		case ZEND_INIT_NS_FCALL_BY_NAME:
			zend = zend_vm_init_ns_fcall_by_name;
		break;

		case ZEND_INIT_STATIC_METHOD_CALL:
			zend = zend_vm_init_static_method_call;
		break;

		case ZEND_INIT_METHOD_CALL:
			zend = zend_vm_init_method_call;
		break;

		case ZEND_NEW:
			zend = zend_vm_new;
		break;

		case ZEND_FETCH_CONSTANT:
			zend = zend_vm_fetch_constant;
		break;

		case ZEND_FETCH_CLASS:
			zend = zend_vm_fetch_class;
		break;

		case ZEND_FETCH_CLASS_CONSTANT:
			zend = zend_vm_fetch_class_constant;
		break;

		case ZEND_DO_FCALL:
			zend = zend_vm_do_fcall;
		break;

		case ZEND_ADD_TRAIT:
			zend = zend_vm_add_trait;
		break;

		case ZEND_ADD_INTERFACE:
			zend = zend_vm_add_interface;
		break;

		case ZEND_FETCH_STATIC_PROP_R:
			zend = zend_vm_fetch_static_prop_r;
		break;

		case ZEND_FETCH_STATIC_PROP_W:
			zend = zend_vm_fetch_static_prop_w;
		break;

		case ZEND_FETCH_STATIC_PROP_RW:
			zend = zend_vm_fetch_static_prop_rw;
		break;

		case ZEND_FETCH_STATIC_PROP_IS:
			zend = zend_vm_fetch_static_prop_is;
		break;

		case ZEND_FETCH_STATIC_PROP_FUNC_ARG:
			zend = zend_vm_fetch_static_prop_func_arg;
		break;

		case ZEND_FETCH_STATIC_PROP_UNSET:
			zend = zend_vm_fetch_static_prop_unset;
		break;

		case ZEND_UNSET_STATIC_PROP:
			zend = zend_vm_unset_static_prop;
		break;

		case ZEND_ISSET_ISEMPTY_STATIC_PROP:
			zend = zend_vm_isset_isempty_static_prop;
		break;

		case ZEND_FETCH_OBJ_R:
			zend = zend_vm_fetch_obj_r;
		break;

		case ZEND_FETCH_OBJ_W:
			zend = zend_vm_fetch_obj_w;
		break;

		case ZEND_FETCH_OBJ_RW:
			zend = zend_vm_fetch_obj_rw;
		break;

		case ZEND_FETCH_OBJ_IS:
			zend = zend_vm_fetch_obj_is;
		break;

		case ZEND_FETCH_OBJ_FUNC_ARG:
			zend = zend_vm_fetch_obj_func_arg;
		break;

		case ZEND_FETCH_OBJ_UNSET:
			zend = zend_vm_fetch_obj_unset;
		break;

		case ZEND_ASSIGN_OBJ:
			zend = zend_vm_assign_obj;
		break;

		case ZEND_ISSET_ISEMPTY_PROP_OBJ:
			zend = zend_vm_isset_isempty_prop_obj;
		break;
	}

	if (zend) {
		return zend(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
}

#define UOPZ_VM_DISPATCH() return _uopz_vm_dispatch(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU)
#define UOPZ_VM_RETURN()   return ZEND_USER_OPCODE_RETURN
#define UOPZ_VM_CONTINUE() return ZEND_USER_OPCODE_CONTINUE
#define UOPZ_VM_NEXT(e, n)    do { \
	if (e) { \
		UOPZ_OPLINE = EX(opline) + (n); \
	} else { \
		UOPZ_OPLINE = opline + (n); \
	} \
	\
	UOPZ_VM_CONTINUE(); \
} while(0)
#define UOPZ_VM_LEAVE()    return ZEND_USER_OPCODE_LEAVE

int uopz_vm_exit(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *estatus;
	zend_free_op free_op1;

	if (UOPZ(exit)) {
		UOPZ_VM_DISPATCH();
	}

	if (opline->op1_type != IS_UNUSED) {
		estatus = uopz_get_zval(
				opline,
				opline->op1_type,
				&opline->op1,
				execute_data,
				&free_op1, BP_VAR_R);

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

		if (free_op1) {
			zval_ptr_dtor_nogc(free_op1);
		}

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

/* {{{ */
/*
	It appears as if INIT_FCALL will not be generated while uopz sets compiler flags to
	ignore internal and user function call optimization, which we must do ...

	It remains because it might be the case that opcache reintroduces init_fcall as part
	of some optimization.
*/
/* }}} */

int uopz_vm_init_fcall(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ LCOV_EXCL_START */
	UOPZ_USE_OPLINE;
	zval *name;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	name = uopz_get_zval(
		opline,
		opline->op2_type,
		&opline->op2,
		execute_data,
		&free_op2, BP_VAR_R);

	fbc = (zend_function*) zend_hash_find_ptr(EG(function_table), Z_STR_P(name));

	if (!fbc) {
		UOPZ_SAVE_OPLINE();
		zend_throw_error(NULL, "Call to undefined function %s()", Z_STRVAL_P(name));
		UOPZ_HANDLE_EXCEPTION();	
	}

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		/* TODO immutable for 7.3 */
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame_ex(
		opline->op1.num, 
		ZEND_CALL_NESTED_FUNCTION, fbc, opline->extended_value, NULL, NULL);
	call->prev_execute_data = EX(call);
	EX(call) = call;

	UOPZ_VM_NEXT(0, 1);
} /* LCOV_EXCL_STOP }}} */

int uopz_vm_init_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *name;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	name = uopz_get_zval(
		opline,
		opline->op2_type,
		&opline->op2,
		execute_data,
		&free_op2, BP_VAR_R);

	fbc = (zend_function*) zend_hash_find_ptr(EG(function_table), Z_STR_P(name+1));

	if (!fbc) {
		UOPZ_SAVE_OPLINE();
		zend_throw_error(NULL, "Call to undefined function %s()", Z_STRVAL_P(name));
		UOPZ_HANDLE_EXCEPTION();
	}

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		/* TODO immutable for 7.3 */
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame(
		ZEND_CALL_NESTED_FUNCTION, 
		fbc, opline->extended_value, NULL, NULL);
	call->prev_execute_data = EX(call);
	EX(call) = call;
	
	UOPZ_VM_NEXT(0, 1);
} /* }}} */

int uopz_vm_init_ns_fcall_by_name(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *name;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	name = uopz_get_zval(
		opline,
		opline->op2_type,
		&opline->op2,
		execute_data,
		&free_op2, BP_VAR_R) + 1;

	fbc = (zend_function*) zend_hash_find_ptr(EG(function_table), Z_STR_P(name));

	if (!fbc) {
		name++;
		fbc = zend_hash_find_ptr(EG(function_table), Z_STR_P(name));
		if (!fbc) {
			UOPZ_SAVE_OPLINE();
			zend_throw_error(NULL,
				"Call to undefined function %s()",
				Z_STRVAL_P(EX_CONSTANT(opline->op2)));
			UOPZ_HANDLE_EXCEPTION();
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
		fbc, opline->extended_value, NULL, NULL);
	call->prev_execute_data = EX(call);
	EX(call) = call;

	UOPZ_VM_NEXT(0, 1);
} /* }}} */

int uopz_vm_init_static_method_call(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zend_object *object = NULL;
	zval *method;
	zend_class_entry *ce;
	zend_function *fbc;
	zend_execute_data *call;
	zend_free_op free_op2;

	UOPZ_SAVE_OPLINE();

	if (opline->op1_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(opline->op1)), &object, &ce) != SUCCESS) {
			UOPZ_VM_DISPATCH();
		}
	} else if (opline->op1_type == IS_UNUSED) {
		ce = zend_fetch_class(NULL, opline->op1.num);

		if (uopz_find_mock(ce->name, &object, &ce) != SUCCESS) {
			UOPZ_VM_DISPATCH();
		}
	} else {
		ce = Z_CE_P(EX_VAR(opline->op1.var));

		if (uopz_find_mock(ce->name, &object, &ce) != SUCCESS) {
			UOPZ_VM_DISPATCH();
		}
	}

	if (opline->op2_type != IS_UNUSED) {
		method = uopz_get_zval(
			opline,
			opline->op2_type,
			&opline->op2,
			execute_data,
			&free_op2, BP_VAR_R);

		if (opline->op2_type != IS_CONST) {
			if (Z_TYPE_P(method) != IS_STRING) {
				do {
					if (opline->op2_type & (IS_VAR|IS_CV) && Z_ISREF_P(method)) {
						method = Z_REFVAL_P(method);

						if (Z_TYPE_P(method) == IS_STRING) {
							break;
						}
					}

					UOPZ_VM_DISPATCH();
				} while (0);
			}
		}

		if (ce->get_static_method) {
			fbc = ce->get_static_method(ce, Z_STR_P(method));
		} else {
			fbc = zend_std_get_static_method(ce, 
				Z_STR_P(method), 
				((opline->op2_type == IS_CONST) ? 
					(EX_CONSTANT(opline->op2) + 1) : NULL));
		}

		if (fbc == NULL) {
			UOPZ_VM_DISPATCH();
		}

		if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
			fbc->op_array.run_time_cache = 
				zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
			memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
		}

		if (opline->op2_type != IS_CONST) {
			if (free_op2) {
				zval_ptr_dtor_nogc(free_op2);
			}
		}
	} else {
		if (ce->constructor == NULL) {
			zend_throw_error(NULL, 
				"Cannot call constructor");
			UOPZ_HANDLE_EXCEPTION();
		}

		if (Z_TYPE(EX(This)) == IS_OBJECT && Z_OBJ(EX(This))->ce != ce->constructor->common.scope && (ce->constructor->common.fn_flags & ZEND_ACC_PRIVATE)) {
			zend_throw_error(NULL, 
				"Cannot call private %s::__construct()", ZSTR_VAL(ce->name));
			UOPZ_HANDLE_EXCEPTION();
		}

		fbc = ce->constructor;

		if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
			fbc->op_array.run_time_cache = 
				zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
			memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
		}
	}

	if (!(fbc->common.fn_flags & ZEND_ACC_STATIC)) {
		if (Z_TYPE(EX(This)) == IS_OBJECT) {
			if (!object) {
				object = Z_OBJ(EX(This));
				ce = object->ce;
			}
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
				UOPZ_HANDLE_EXCEPTION();
			}
		}
	}

	if (opline->op1_type == IS_UNUSED) {
		if ((opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_PARENT ||
		    (opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_SELF) {
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
		fbc, opline->extended_value, ce, object);

	call->prev_execute_data = EX(call);
	EX(call) = call;

	UOPZ_VM_NEXT(0, 1);
} /* }}} */

int uopz_vm_init_method_call(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *object;
	zend_object *obj;
	zend_function *fbc;
	zend_class_entry *scope;
	zend_execute_data *call;
	zval *method;
	zend_class_entry *mock;
	uint32_t info;
	zend_free_op free_op1;
	zend_free_op free_op2;

	UOPZ_SAVE_OPLINE();

	object = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (!object || Z_TYPE_P(object) == IS_UNDEF) {
		UOPZ_VM_DISPATCH();
	}

	method = uopz_get_zval(
			opline,
			opline->op2_type,
			&opline->op2,
			execute_data,
			&free_op2, BP_VAR_R);

	if (!method) {
		UOPZ_VM_DISPATCH();
	}

	if (Z_TYPE_P(method) != IS_STRING) {
		do {
			if ((opline->op2_type & (IS_VAR|IS_CV)) && Z_ISREF_P(method)) {
				method = Z_REFVAL_P(method);

				if (Z_TYPE_P(method) == IS_STRING) {
					break;
				}
			}
			
			UOPZ_VM_DISPATCH();
		} while(0);
	}

	if (opline->op1_type != IS_UNUSED && Z_TYPE_P(object) != IS_OBJECT) {
		do {
			if (opline->op1_type == IS_CONST || Z_TYPE_P(object) != IS_OBJECT) {
				if ((opline->op1_type & (IS_VAR|IS_CV)) && Z_ISREF_P(object)) {
					object = Z_REFVAL_P(object);

					if (Z_TYPE_P(object) == IS_OBJECT) {
						break;
					}
				
				}
			}

			UOPZ_VM_DISPATCH();
		} while (0);
	}

	obj = Z_OBJ_P(object);
	scope = obj->ce;

	if (obj->handlers->get_method == NULL) {
		UOPZ_VM_DISPATCH();
	}

	fbc = obj->handlers->get_method(&obj, 
			Z_STR_P(method), 
			((opline->op2_type == IS_CONST) ? 
				(EX_CONSTANT(opline->op2) + 1) : NULL));

	if (!fbc) {
		UOPZ_VM_DISPATCH();
	}

#if PHP_VERSION_ID >= 70300
	if (opline->op2_type != IS_CONST) {
		if (free_op2) {
			zval_ptr_dtor_nogc(free_op2);
		}
	}
#endif

	if (uopz_find_mock(fbc->common.scope->name, &obj, &mock) == SUCCESS) {
		uopz_find_method(
			mock, Z_STR_P(method), &fbc);
	}

	info = ZEND_CALL_NESTED_FUNCTION;

	if ((fbc->common.fn_flags & ZEND_ACC_STATIC) != 0) {
		obj = NULL;
#if PHP_VERSION_ID >= 70300
		if (free_op1) {
			zval_ptr_dtor_nogc(free_op1);
		}

		if ((opline->op1_type & (IS_VAR|IS_TMP_VAR)) && EG(exception)) {
			UOPZ_HANDLE_EXCEPTION();
		}
#endif
	} else if (opline->op1_type & (IS_VAR|IS_TMP_VAR|IS_CV)) {
		info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_RELEASE_THIS;

#if PHP_VERSION_ID >= 70300
		if (opline->op1_type == IS_CV) {
			GC_ADDREF(obj);
		} else if (free_op1 != object) {
			GC_ADDREF(obj);
			zval_ptr_dtor_nogc(free_op1);
		}
#else
		GC_ADDREF(obj);
#endif
	}

#if PHP_VERSION_ID < 70300
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);
	}

	if ((opline->op1_type & (IS_VAR|IS_TMP_VAR)) && EG(exception)) {
		UOPZ_HANDLE_EXCEPTION();
	}
#endif

	if (fbc->type == ZEND_USER_FUNCTION && !fbc->op_array.run_time_cache) {
		fbc->op_array.run_time_cache = 
			zend_arena_alloc(&CG(arena), fbc->op_array.cache_size);
		memset(fbc->op_array.run_time_cache, 0, fbc->op_array.cache_size);
	}

	call = zend_vm_stack_push_call_frame(info, 
		fbc, opline->extended_value, scope, obj);

	call->prev_execute_data = EX(call);
	EX(call) = call;

	UOPZ_VM_NEXT(0, 1);
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
				EX_CONSTANT(opline->op1) + 1, 
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
			opline->extended_value, NULL, NULL);

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
			opline->extended_value, NULL, NULL);
	} else {
		if (constructor->type == ZEND_USER_FUNCTION && !constructor->op_array.run_time_cache) {
			constructor->op_array.run_time_cache = 
				zend_arena_alloc(&CG(arena), constructor->op_array.cache_size);
			memset(constructor->op_array.run_time_cache, 0, constructor->op_array.cache_size);
		}

		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION | ZEND_CALL_RELEASE_THIS | ZEND_CALL_CTOR,
			constructor,
			opline->extended_value,
			ce,
			Z_OBJ_P(result));

		Z_ADDREF_P(result);
	}

	call->prev_execute_data = EX(call);
	EX(call) = call;

	UOPZ_VM_NEXT(0, 1);
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

	UOPZ_VM_LEAVE();
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
	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_fetch_constant(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
#if PHP_VERSION_ID >= 70300
	CACHE_PTR(opline->extended_value, NULL);
#else
	if (CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(opline->op2)))) {
		CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(opline->op2)), NULL);
	}
#endif

	UOPZ_VM_DISPATCH();
} /* }}} */

int uopz_vm_fetch_class_constant(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zend_class_entry *ce, *scope;
	zend_class_constant *c;
	zval *value;

	UOPZ_SAVE_OPLINE();
	
	if (opline->op1_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(opline->op1)), NULL, &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(EX_CONSTANT(opline->op1)), 
				EX_CONSTANT(opline->op1) + 1, 
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);

			if (!ce) {
				ZVAL_UNDEF(EX_VAR(opline->result.var));

				UOPZ_HANDLE_EXCEPTION();
			}
		}
	} else {
		if (opline->op1_type == IS_UNUSED) {
			ce = zend_fetch_class(NULL, opline->op1.num);
			if (!ce) {
				ZVAL_UNDEF(EX_VAR(opline->result.var));

				UOPZ_HANDLE_EXCEPTION();
			}
			uopz_find_mock(ce->name, NULL, &ce);
		} else {
			ce = 
				Z_CE_P(EX_VAR(opline->op1.var));
			uopz_find_mock(ce->name, NULL, &ce);
		}
	}

	if ((c = zend_hash_find_ptr(&ce->constants_table, Z_STR_P(EX_CONSTANT(opline->op2))))) {
		scope = EX(func)->op_array.scope;
		if (!zend_verify_const_access(c, scope)) {
			zend_throw_error(NULL, 
				"Cannot access const %s::%s", 
				ZSTR_VAL(ce->name), Z_STRVAL_P(EX_CONSTANT(opline->op2)));
			ZVAL_UNDEF(EX_VAR(opline->result.var));

			UOPZ_HANDLE_EXCEPTION();
		}

		value = &c->value;
		if (Z_CONSTANT_P(value)) {
			zval_update_constant_ex(value, c->ce);
			if (EG(exception)) {
				ZVAL_UNDEF(EX_VAR(opline->result.var));

				UOPZ_HANDLE_EXCEPTION();
			}
		}
	} else {
		zend_throw_error(NULL, 
			"Undefined class constant '%s'", 
			Z_STRVAL_P(EX_CONSTANT(opline->op2)));
		ZVAL_UNDEF(EX_VAR(opline->result.var));

		UOPZ_HANDLE_EXCEPTION();
	}

#ifdef ZTS
	if (ce->type == ZEND_INTERNAL_CLASS) {
		ZVAL_DUP(EX_VAR(opline->result.var), value);
	} else {
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}
#else
	ZVAL_COPY(EX_VAR(opline->result.var), value);
#endif

	UOPZ_VM_NEXT(0, 1);
} /* }}} */

int uopz_vm_fetch_class(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zval *name;
	zend_free_op free_op2;

	UOPZ_SAVE_OPLINE();

	if (opline->op2_type == IS_UNUSED) {
		Z_CE_P(EX_VAR(opline->result.var)) = 
			zend_fetch_class(NULL, opline->op1.num);
		
		if (EG(exception)) {
			UOPZ_HANDLE_EXCEPTION();
		}

		uopz_find_mock(
			Z_CE_P(EX_VAR(opline->result.var))->name,
			NULL, 
			&Z_CE_P(EX_VAR(opline->result.var)));

		UOPZ_VM_NEXT(0, 1);
	} else if (opline->op2_type == IS_CONST) {
		name = uopz_get_zval(
			opline,
			opline->op2_type,
			&opline->op2,
			execute_data,
			&free_op2, BP_VAR_R);

		if (uopz_find_mock(Z_STR_P(name), NULL, &Z_CE_P(EX_VAR(opline->result.var))) != SUCCESS) {
			Z_CE_P(EX_VAR(opline->result.var)) = zend_fetch_class_by_name(
									Z_STR_P(name), 
									name + 1, 
									opline->op1.num);
		}
	} else {
		name = uopz_get_zval(
			opline,
			opline->op2_type,
			&opline->op2,
			execute_data,
			&free_op2, BP_VAR_R);
_uopz_vm_fetch_class_try:
		if (Z_TYPE_P(name) == IS_OBJECT) {
			if (uopz_find_mock(Z_OBJCE_P(name)->name, NULL, &Z_CE_P(EX_VAR(opline->result.var))) != SUCCESS) {
				Z_CE_P(EX_VAR(opline->result.var)) = Z_OBJCE_P(name);	
			}
		} else if (Z_TYPE_P(name) == IS_STRING) {
			if (uopz_find_mock(Z_STR_P(name), NULL, &Z_CE_P(EX_VAR(opline->result.var))) != SUCCESS) {
				Z_CE_P(EX_VAR(opline->result.var)) = zend_fetch_class(Z_STR_P(name), opline->op1.num);
			}
		} else if (opline->op2_type & (IS_VAR|IS_CV) && Z_TYPE_P(name) == IS_REFERENCE) {
			name = Z_REFVAL_P(name);
			goto _uopz_vm_fetch_class_try;
		} else {
			if (opline->op2_type == IS_CV && Z_TYPE_P(name) == IS_UNDEF) {
				if (EG(exception)) {
					UOPZ_HANDLE_EXCEPTION();
				}
			}
			zend_throw_error(NULL, "Class name must be a valid object or a string");
		}
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);
	}

	UOPZ_VM_NEXT(1, 1);
} /* }}} */

int uopz_vm_add_trait(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zend_class_entry *ce = Z_CE_P(EX_VAR(opline->op1.var)), 
                         *trait;
	zval *name = EX_CONSTANT(opline->op2);

	UOPZ_SAVE_OPLINE();

	if (uopz_find_mock(Z_STR_P(name), NULL, &trait) != SUCCESS) {
		trait = zend_fetch_class_by_name(
				Z_STR_P(name), 
				name + 1, 
				ZEND_FETCH_CLASS_TRAIT);

		if (!trait) {
			UOPZ_HANDLE_EXCEPTION();
		}
	}

	if (!(trait->ce_flags & ZEND_ACC_TRAIT)) {
		zend_error_noreturn(E_ERROR, "%s cannot use %s - it is not a trait", ZSTR_VAL(ce->name), ZSTR_VAL(trait->name));
	}

	zend_do_implement_trait(ce, trait);

	UOPZ_VM_NEXT(1, 1);
} /* }}} */

int uopz_vm_add_interface(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	UOPZ_USE_OPLINE;
	zend_class_entry *ce = Z_CE_P(EX_VAR(opline->op1.var)), 
                         *iface;
	zval *name = EX_CONSTANT(opline->op2);

	UOPZ_SAVE_OPLINE();

	if (uopz_find_mock(Z_STR_P(name), NULL, &iface) != SUCCESS) {
		iface = zend_fetch_class_by_name(
				Z_STR_P(name), 
				name + 1, 
				ZEND_FETCH_CLASS_TRAIT);

		if (!iface) {
			UOPZ_HANDLE_EXCEPTION();
		}
	}

	if (!(iface->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error_noreturn(E_ERROR, "%s cannot implement %s - it is not an interface", ZSTR_VAL(ce->name), ZSTR_VAL(iface->name));
	}

	zend_do_implement_interface(ce, iface);

	UOPZ_VM_NEXT(1, 1);
} /* }}} */

/* {{{ */
int uopz_vm_fetch_static_prop_r(UOPZ_OPCODE_HANDLER_ARGS) {
	return uopz_vm_fetch_static_helper(BP_VAR_R UOPZ_OPCODE_HANDLER_ARGS_CC);
}

int uopz_vm_fetch_static_prop_w(UOPZ_OPCODE_HANDLER_ARGS) {
	return uopz_vm_fetch_static_helper(BP_VAR_W UOPZ_OPCODE_HANDLER_ARGS_CC);
}

int uopz_vm_fetch_static_prop_rw(UOPZ_OPCODE_HANDLER_ARGS) {
	return uopz_vm_fetch_static_helper(BP_VAR_RW UOPZ_OPCODE_HANDLER_ARGS_CC);
}

int uopz_vm_fetch_static_prop_is(UOPZ_OPCODE_HANDLER_ARGS) {
	return uopz_vm_fetch_static_helper(BP_VAR_IS UOPZ_OPCODE_HANDLER_ARGS_CC);
}

int uopz_vm_fetch_static_prop_func_arg(UOPZ_OPCODE_HANDLER_ARGS) {
#if PHP_VERSION_ID >= 70300
	return uopz_vm_fetch_static_helper(
		(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF) ?
			BP_VAR_W : BP_VAR_R
		 UOPZ_OPCODE_HANDLER_ARGS_CC);
#else
	if (uopz_is_by_ref_func_arg_fetch(EX(opline), EX(call))) {
		return uopz_vm_fetch_static_helper(BP_VAR_W UOPZ_OPCODE_HANDLER_ARGS_CC);
	} else {
		return uopz_vm_fetch_static_helper(BP_VAR_R UOPZ_OPCODE_HANDLER_ARGS_CC);
	}
#endif
}

int uopz_vm_fetch_static_prop_unset(UOPZ_OPCODE_HANDLER_ARGS) {
	return uopz_vm_fetch_static_helper(BP_VAR_UNSET UOPZ_OPCODE_HANDLER_ARGS_CC);
}

int uopz_vm_unset_static_prop(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1;
#if PHP_VERSION_ID >= 70300
	zval *vname;
	zend_string *pname, *tname;
#else
	zval tmp, *vname;
#endif
	zend_class_entry *ce;

	UOPZ_SAVE_OPLINE();

#if PHP_VERSION_ID < 70300
	ZVAL_UNDEF(&tmp);

	vname = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (opline->op1_type != IS_CONST && Z_TYPE_P(vname) != IS_STRING) {
		if (opline->op1_type == IS_CV && Z_TYPE_P(vname) == IS_UNDEF) {
			UOPZ_VM_DISPATCH();
		}

		ZVAL_STR(&tmp, zval_get_string(vname));
		vname = &tmp;
	}
#endif

	if (opline->op2_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(opline->op2)), NULL, &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(EX_CONSTANT(opline->op2)), 
				EX_CONSTANT(opline->op2) + 1, 
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			if (ce == NULL) {
				/* FREE_UNFETCHED_OP */
				UOPZ_HANDLE_EXCEPTION();
			}
		}
	} else if (opline->op2_type == IS_UNUSED) {
		ce = zend_fetch_class(NULL, 
			opline->op2.num);
		uopz_find_mock(ce->name, NULL, &ce);
	} else {
		ce = Z_CE_P(
			EX_VAR(opline->op2.var));
		uopz_find_mock(ce->name, NULL, &ce);
	}
#if PHP_VERSION_ID >= 70300
	vname = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (opline->op1_type == IS_CONST) {
		pname = Z_STR_P(vname);
		tname = NULL;
	} else if (Z_TYPE_P(vname) == IS_STRING) {
		pname = Z_STR_P(vname);
		tname = NULL;
	} else {
		if (opline->op1_type == IS_CV && Z_TYPE_P(vname) == IS_UNDEF) {
			/* vname = GET_OP1_UNDEF_CV(vname, BP_VAR_R); */
		}
		pname = zval_get_tmp_string(vname, &tname);
	}
	zend_std_unset_static_property(ce, pname);
	
	if (tname) {
		zend_tmp_string_release(tname);
	}
#else
	zend_std_unset_static_property(ce, Z_STR_P(vname));

	if (Z_TYPE(tmp) != IS_UNDEF) {
		zend_string_release(Z_STR(tmp));
	}
#endif

	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (EG(exception)) {
		UOPZ_HANDLE_EXCEPTION();
	}

	UOPZ_VM_NEXT(1, 1);
}

int uopz_vm_isset_isempty_static_prop(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zval *value;
	int result;
	zend_free_op free_op1;
#if PHP_VERSION_ID >= 70300
	zval *vname;
	zend_string *pname, *tname;
#else
	zval tmp, *vname;
#endif
	zend_class_entry *ce;

	UOPZ_SAVE_OPLINE();

#if PHP_VERSION_ID < 70300
	ZVAL_UNDEF(&tmp);

	vname = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (opline->op1_type != IS_CONST && Z_TYPE_P(vname) != IS_STRING) {
		ZVAL_STR(&tmp, zval_get_string(vname));
		vname = &tmp;
	}
#endif

	if (opline->op2_type == IS_CONST) {
		if (uopz_find_mock(Z_STR_P(EX_CONSTANT(opline->op2)), NULL, &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(EX_CONSTANT(opline->op2)), 
				EX_CONSTANT(opline->op2) + 1, 
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			if (ce == NULL) {
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				UOPZ_HANDLE_EXCEPTION();
			}
		}
	} else {
		if (opline->op2_type == IS_UNUSED) {
			ce = zend_fetch_class(NULL, 
				opline->op2.num);
			uopz_find_mock(ce->name, NULL, &ce);
		} else {
			ce = Z_CE_P(
				EX_VAR(opline->op2.var));
			uopz_find_mock(ce->name, NULL, &ce);
		}
	}

#if PHP_VERSION_ID >= 70300
	vname = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (opline->op1_type == IS_CONST) {
		pname = Z_STR_P(vname);
	} else {
		pname = zval_get_tmp_string(vname, &tname);
	}

	value = zend_std_get_static_property(ce, pname, 1);

	if (opline->op1_type != IS_CONST) {
		zend_tmp_string_release(tname);
	}
#else
	value = zend_std_get_static_property(ce, Z_STR_P(vname), 1);

	if (Z_TYPE(tmp) != IS_UNDEF) {
		zend_string_release(Z_STR(tmp));
	}
#endif
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		result = value && Z_TYPE_P(value) > IS_NULL &&
		    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL);
	} else {
		result = !value || !zend_is_true(value);
	}
	ZVAL_BOOL(EX_VAR(opline->result.var), result);

	UOPZ_VM_NEXT(1, 1);
}
/* }}} */

int uopz_vm_fetch_static_helper(int type UOPZ_OPCODE_HANDLER_ARGS_DC) { /* {{{ */
	UOPZ_USE_OPLINE;
	zend_free_op free_op1;
#if PHP_VERSION_ID >= 70300
	zval *vname;
	zend_string *pname, *tname;
#else
	zval tmp, *vname;
#endif
	zval *rv;
	zend_class_entry *ce;

	UOPZ_SAVE_OPLINE();

#if PHP_VERSION_ID < 70300
	ZVAL_UNDEF(&tmp);

	vname = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (opline->op1_type != IS_CONST && Z_TYPE_P(vname) != IS_STRING) {
		ZVAL_STR(&tmp, zval_get_string(vname));
		vname = &tmp;
	}
#endif

	if (opline->op2_type == IS_CONST) {
		zval *clazz = EX_CONSTANT(opline->op2);
		if (uopz_find_mock(Z_STR_P(clazz), NULL, &ce) != SUCCESS) {
			ce = zend_fetch_class_by_name(
				Z_STR_P(clazz), clazz + 1, 
				ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			if (ce == NULL) {
				UOPZ_HANDLE_EXCEPTION();
			}
		}
	} else {
		if (opline->op2_type == IS_UNUSED) {
			ce = zend_fetch_class(NULL,
				opline->op2.num);
			if (ce == NULL) {
				UOPZ_HANDLE_EXCEPTION();
			}
			uopz_find_mock(ce->name, NULL, &ce);
		} else {
			ce = Z_CE_P(
				EX_VAR(opline->op2.var));

			uopz_find_mock(ce->name, NULL, &ce);
		}
	}

#if PHP_VERSION_ID >= 70300
	vname = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_R);

	if (opline->op1_type == IS_CONST) {
		pname = Z_STR_P(vname); 
	} else if (Z_TYPE_P(vname) == IS_STRING) {
		pname = Z_STR_P(vname);
		tname = NULL;
	} else {
		if (opline->op1_type == IS_CV && Z_TYPE_P(vname) == IS_UNDEF) {
			/* uopz_undefined_cv(opline->op1.var) */
		}
		pname = zval_get_tmp_string(vname, &tname);
	}

	rv = zend_std_get_static_property(ce, pname, type == BP_VAR_IS);

	if (opline->op1_type != IS_CONST) {
		zend_tmp_string_release(tname);
	}
#else
	rv = zend_std_get_static_property(ce, Z_STR_P(vname), type == BP_VAR_IS);

	if (Z_TYPE(tmp) != IS_UNDEF) {
		zend_string_release(Z_STR(tmp));
	}
#endif
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (rv == NULL) {
		if (EG(exception)) {
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			UOPZ_HANDLE_EXCEPTION();
		} else {
			rv = &EG(uninitialized_zval);
		}
	}

	if (type == BP_VAR_R || type == BP_VAR_IS) {
#if PHP_VERSION_ID >= 70300
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), rv);
#else
		ZVAL_COPY_UNREF(EX_VAR(opline->result.var), rv);
#endif
	} else {
		ZVAL_INDIRECT(EX_VAR(opline->result.var), rv);
	}

	if (EG(exception)) {
		UOPZ_HANDLE_EXCEPTION();
	}

	UOPZ_VM_NEXT(0, 1);
} /* }}} */

/* {{{ */
static zend_always_inline int uopz_vm_fetch_obj_helper(zval **container, uint32_t cfetch, zval **offset, uint32_t ofetch, zval *mock, zend_free_op *free_op1, zend_free_op *free_op2 UOPZ_OPCODE_HANDLER_ARGS_DC) {
	UOPZ_USE_OPLINE;
	zend_object *object = NULL;
	zend_class_entry *ce;

	*container = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			free_op1, cfetch);

	if (opline->op1_type == IS_UNUSED && (!*container || Z_TYPE_P(*container) == IS_UNDEF)) {
		return FAILURE;
	}

	*offset = uopz_get_zval(
			opline,
			opline->op2_type,
			&opline->op2,
			execute_data,
			free_op2, ofetch);

	if ((opline->op1_type == IS_CONST) || 
	    (opline->op1_type != IS_UNUSED && Z_TYPE_P(*container) != IS_OBJECT)) {
		if ((opline->op1_type & (IS_VAR|IS_CV)) && Z_ISREF_P(*container)) {
			*container = Z_REFVAL_P(*container);
			if (!Z_TYPE_P(*container) != IS_OBJECT) {
				return FAILURE;
			}
		} else {
			return FAILURE;
		}
	}

	if (uopz_find_mock(Z_OBJCE_P(*container)->name, &object, &ce) != SUCCESS) {
		return FAILURE;
	}

	if (!object) {
		return FAILURE;
	}

	ZVAL_OBJ(mock, object);

	*container = mock;
	
	return SUCCESS;
}

int uopz_vm_fetch_obj_r(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2;
	zval mock, *container, *offset;
	zval *retval;

	UOPZ_SAVE_OPLINE();

	if (uopz_vm_fetch_obj_helper(&container, BP_VAR_R, &offset, BP_VAR_R, &mock, &free_op1, &free_op2 UOPZ_OPCODE_HANDLER_ARGS_CC) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	retval = Z_OBJ_HT(mock)->read_property(&mock, offset, BP_VAR_R, NULL, EX_VAR(opline->result.var));
	
	if (retval != EX_VAR(opline->result.var)) {
#if PHP_VERSION_ID >= 70300
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
#else
		ZVAL_COPY_UNREF(EX_VAR(opline->result.var), retval);
#endif
	} else if(Z_ISREF_P(retval)) {
		zend_unwrap_reference(retval);
	}

	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);	
	}

	UOPZ_VM_NEXT(1, 1);
}

int uopz_vm_fetch_obj_w(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2;
	zval mock, *container, *offset;
	zval *retval;

	UOPZ_SAVE_OPLINE();

	if (uopz_vm_fetch_obj_helper(&container, BP_VAR_W, &offset, BP_VAR_R, &mock, &free_op1, &free_op2 UOPZ_OPCODE_HANDLER_ARGS_CC) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	retval = Z_OBJ_HT(mock)->read_property(&mock, offset, BP_VAR_W, NULL, EX_VAR(opline->result.var));
	
	if (retval != EX_VAR(opline->result.var)) {
		ZVAL_INDIRECT(EX_VAR(opline->result.var), retval);
	} else if(Z_ISREF_P(retval) && Z_REFCOUNT_P(retval) == 1) {
		ZVAL_UNREF(retval);
	}

	/* TODO FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY */
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);	
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);	
	}

	UOPZ_VM_NEXT(1, 1);
}

int uopz_vm_fetch_obj_rw(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2;
	zval mock, *container, *offset;
	zval *retval;

	UOPZ_SAVE_OPLINE();

	if (uopz_vm_fetch_obj_helper(&container, BP_VAR_RW, &offset, BP_VAR_R, &mock, &free_op1, &free_op2 UOPZ_OPCODE_HANDLER_ARGS_CC) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	retval = Z_OBJ_HT(mock)->read_property(&mock, offset, BP_VAR_RW, NULL, EX_VAR(opline->result.var));
	
	if (retval != EX_VAR(opline->result.var)) {
		ZVAL_INDIRECT(EX_VAR(opline->result.var), retval);
	} else if(Z_ISREF_P(retval) && Z_REFCOUNT_P(retval) == 1) {
		ZVAL_UNREF(retval);
	}

	/* TODO FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY */
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);	
	}

	UOPZ_VM_NEXT(1, 1);
}

int uopz_vm_fetch_obj_is(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2;
	zval mock, *container, *offset;
	zval *retval;

	UOPZ_SAVE_OPLINE();

	if (uopz_vm_fetch_obj_helper(&container, BP_VAR_IS, &offset, BP_VAR_R, &mock, &free_op1, &free_op2 UOPZ_OPCODE_HANDLER_ARGS_CC) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	retval = Z_OBJ_HT(mock)->read_property(&mock, offset, BP_VAR_IS, NULL, EX_VAR(opline->result.var));

	if (retval != EX_VAR(opline->result.var)) {
		ZVAL_COPY(EX_VAR(opline->result.var), retval);
	}

	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);	
	}
	
	UOPZ_VM_NEXT(1, 1);
}

int uopz_vm_fetch_obj_func_arg(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
#if PHP_VERSION_ID >= 70300
	if (ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF) {
#else
	if (uopz_is_by_ref_func_arg_fetch(opline, EX(call))) {
#endif
		if (opline->op1_type & (IS_CONST|IS_TMP_VAR)) {
			UOPZ_VM_DISPATCH();
		}
		return uopz_vm_fetch_obj_w(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	} else {
		return uopz_vm_fetch_obj_r(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}
}

int uopz_vm_fetch_obj_unset(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2;
	zval mock, *container, *offset;
	zval *retval;

	UOPZ_SAVE_OPLINE();

	if (uopz_vm_fetch_obj_helper(&container, BP_VAR_UNSET, &offset, BP_VAR_R, &mock, &free_op1, &free_op2 UOPZ_OPCODE_HANDLER_ARGS_CC) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	retval = Z_OBJ_HT(mock)->read_property(&mock, offset, BP_VAR_UNSET, NULL, EX_VAR(opline->result.var));
	
	if (retval != EX_VAR(opline->result.var)) {
		ZVAL_INDIRECT(EX_VAR(opline->result.var), retval);
	} else if(Z_ISREF_P(retval) && Z_REFCOUNT_P(retval) == 1) {
		ZVAL_UNREF(retval);
	}

	/* TODO FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY */
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);	
	}
}

int uopz_vm_isset_isempty_prop_obj(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2;
	zval mock, *container, *offset;
	int result;

	UOPZ_SAVE_OPLINE();

	if (uopz_vm_fetch_obj_helper(&container, BP_VAR_IS, &offset, BP_VAR_R, &mock, &free_op1, &free_op2 UOPZ_OPCODE_HANDLER_ARGS_CC) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	if (!Z_OBJ_HT(mock)->has_property) {
		result = (opline->extended_value & ZEND_ISEMPTY);
	} else {
		result = (opline->extended_value & ZEND_ISEMPTY) ^ Z_OBJ_HT(mock)->has_property(
				&mock, offset, 
				(opline->extended_value & ZEND_ISEMPTY), 
				NULL);
	}

	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);	
	}

	ZVAL_BOOL(EX_VAR(opline->result.var), result);

	UOPZ_VM_NEXT(1, 1);
}

int uopz_vm_assign_obj(UOPZ_OPCODE_HANDLER_ARGS) {
	UOPZ_USE_OPLINE;
	zend_free_op free_op1, free_op2, free_op_data;
	zval *object, *property, *value, tmp;
	zend_object *obj;
	zend_class_entry *ce;

	UOPZ_SAVE_OPLINE();

	object = uopz_get_zval(
			opline,
			opline->op1_type,
			&opline->op1,
			execute_data,
			&free_op1, BP_VAR_W);

	if (opline->op1_type == IS_UNUSED && (!object || Z_TYPE_P(object) == IS_UNDEF)) {
		UOPZ_VM_DISPATCH();
	}

	property = uopz_get_zval(
			opline,
			opline->op2_type,
			&opline->op2,
			execute_data,
			&free_op2, BP_VAR_R);

	value =    uopz_get_zval(
			(opline + 1),
			(opline + 1)->op1_type,
			&(opline + 1)->op1,
			execute_data,
			&free_op_data, BP_VAR_R);
	
	do {
		if (opline->op2_type != IS_UNUSED && Z_TYPE_P(object) != IS_OBJECT) {
			if (Z_ISREF_P(object)) {
				object = Z_REFVAL_P(object);
				if (Z_TYPE_P(object) == IS_OBJECT) {
					break;
				}
			}
			
			UOPZ_VM_DISPATCH();
		}
	} while (0);

	if (uopz_find_mock(Z_OBJCE_P(object)->name, &obj, &ce) != SUCCESS) {
		UOPZ_VM_DISPATCH();
	}

	if (!obj) {
		UOPZ_VM_DISPATCH();
	}

	ZVAL_OBJ(&tmp, obj);

	obj->handlers->write_property(&tmp, property, value, NULL);

	if (RETURN_VALUE_USED(opline)) {
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}

	if (free_op_data) {
		zval_ptr_dtor_nogc(free_op_data);
	}
	
	if (free_op1) {
		zval_ptr_dtor_nogc(free_op1);
	}

	if (free_op2) {
		zval_ptr_dtor_nogc(free_op2);
	}

	UOPZ_VM_NEXT(1, 2);
}
/* }}} */

#endif	/* UOPZ_HANDLERS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
