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

#include "return.h"
#include "hook.h"

#ifdef ZEND_VM_FP_GLOBAL_REG
#	define UOPZ_OPCODE_HANDLER_ARGS
#	define UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU
#else
#	define UOPZ_OPCODE_HANDLER_ARGS zend_execute_data *execute_data
#	define UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU execute_data
#endif


#if PHP_VERSION_ID >= 70100
#	define RETURN_VALUE_USED(opline) ((opline)->result_type != IS_UNUSED)
#else
#	define RETURN_VALUE_USED(opline) (!((opline)->result_type & EXT_TYPE_UNUSED))
#endif

ZEND_EXTERN_MODULE_GLOBALS(uopz);

int uopz_no_exit_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_call_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_constant_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_mock_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_fetch_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_return_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_add_class_handler(UOPZ_OPCODE_HANDLER_ARGS);
#ifdef ZEND_FETCH_CLASS_CONSTANT
int uopz_class_constant_handler(UOPZ_OPCODE_HANDLER_ARGS);
#endif

typedef int (*uopz_opcode_handler_t) (UOPZ_OPCODE_HANDLER_ARGS);

uopz_opcode_handler_t uopz_exit_handler;
uopz_opcode_handler_t uopz_init_fcall_by_name_handler;
uopz_opcode_handler_t uopz_init_fcall_handler;
uopz_opcode_handler_t uopz_init_ns_fcall_by_name_handler;
uopz_opcode_handler_t uopz_init_method_call_handler;
uopz_opcode_handler_t uopz_init_static_method_call_handler;
uopz_opcode_handler_t uopz_new_handler;
uopz_opcode_handler_t uopz_fetch_constant_handler;
uopz_opcode_handler_t uopz_do_fcall_handler;
uopz_opcode_handler_t uopz_fetch_class_handler;
uopz_opcode_handler_t uopz_add_trait_handler;
uopz_opcode_handler_t uopz_add_interface_handler;
#ifdef ZEND_FETCH_CLASS_CONSTANT
uopz_opcode_handler_t uopz_fetch_class_constant_handler;
#endif

#define UOPZ_SET_HANDLER(h, o, n) do { \
	(h) = zend_get_user_opcode_handler((o)); \
	zend_set_user_opcode_handler((o), (n)); \
} while (0)

#define UOPZ_UNSET_HANDLER(h, o) do { \
	zend_set_user_opcode_handler(o, h); \
} while (0)

void uopz_handlers_init(void) {
	UOPZ_SET_HANDLER(uopz_exit_handler,                     ZEND_EXIT,                      uopz_no_exit_handler);
	UOPZ_SET_HANDLER(uopz_init_fcall_by_name_handler,		ZEND_INIT_FCALL_BY_NAME, 		uopz_call_handler);
	UOPZ_SET_HANDLER(uopz_init_fcall_handler,				ZEND_INIT_FCALL, 				uopz_call_handler);
	UOPZ_SET_HANDLER(uopz_init_ns_fcall_by_name_handler,	ZEND_INIT_NS_FCALL_BY_NAME, 	uopz_call_handler);
	UOPZ_SET_HANDLER(uopz_init_method_call_handler,			ZEND_INIT_METHOD_CALL,			uopz_call_handler);
	UOPZ_SET_HANDLER(uopz_init_static_method_call_handler,	ZEND_INIT_STATIC_METHOD_CALL,	uopz_call_handler);
	UOPZ_SET_HANDLER(uopz_new_handler,						ZEND_NEW,						uopz_mock_handler);
	UOPZ_SET_HANDLER(uopz_fetch_constant_handler,			ZEND_FETCH_CONSTANT,			uopz_constant_handler);
	UOPZ_SET_HANDLER(uopz_do_fcall_handler,					ZEND_DO_FCALL,					uopz_return_handler);
#ifdef ZEND_FETCH_CLASS_CONSTANT
	UOPZ_SET_HANDLER(uopz_fetch_class_constant_handler,		ZEND_FETCH_CLASS_CONSTANT,		uopz_class_constant_handler);
#endif
	UOPZ_SET_HANDLER(uopz_fetch_class_handler,				ZEND_FETCH_CLASS,				uopz_fetch_handler);
	UOPZ_SET_HANDLER(uopz_add_trait_handler,				ZEND_ADD_TRAIT,					uopz_add_class_handler);
	UOPZ_SET_HANDLER(uopz_add_interface_handler,			ZEND_ADD_INTERFACE,				uopz_add_class_handler);
}

void uopz_handlers_shutdown(void) {
	UOPZ_UNSET_HANDLER(uopz_exit_handler,					ZEND_EXIT);
	UOPZ_UNSET_HANDLER(uopz_init_fcall_by_name_handler,		ZEND_INIT_FCALL_BY_NAME);
	UOPZ_UNSET_HANDLER(uopz_init_fcall_handler,				ZEND_INIT_FCALL);
	UOPZ_UNSET_HANDLER(uopz_init_ns_fcall_by_name_handler,	ZEND_INIT_NS_FCALL_BY_NAME);
	UOPZ_UNSET_HANDLER(uopz_init_method_call_handler,		ZEND_INIT_METHOD_CALL);
	UOPZ_UNSET_HANDLER(uopz_init_static_method_call_handler,ZEND_INIT_STATIC_METHOD_CALL);
	UOPZ_UNSET_HANDLER(uopz_new_handler,					ZEND_NEW);
	UOPZ_UNSET_HANDLER(uopz_fetch_constant_handler,			ZEND_FETCH_CONSTANT);
	UOPZ_UNSET_HANDLER(uopz_do_fcall_handler,				ZEND_DO_FCALL);
#ifdef ZEND_FETCH_CLASS_CONSTANT
	UOPZ_UNSET_HANDLER(uopz_fetch_class_constant_handler,	ZEND_FETCH_CLASS_CONSTANT);
#endif
	UOPZ_UNSET_HANDLER(uopz_fetch_class_handler,			ZEND_FETCH_CLASS);
	UOPZ_UNSET_HANDLER(uopz_add_trait_handler,				ZEND_ADD_TRAIT);
	UOPZ_UNSET_HANDLER(uopz_add_interface_handler,			ZEND_ADD_INTERFACE);
}

int uopz_no_exit_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	if (UOPZ(exit)) {
		if (uopz_exit_handler)
			return uopz_exit_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

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

int uopz_call_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	switch (EX(opline)->opcode) {
		case ZEND_INIT_FCALL_BY_NAME:
		case ZEND_INIT_FCALL:
		case ZEND_INIT_NS_FCALL_BY_NAME: {
			zval *function_name = EX_CONSTANT(EX(opline)->op2);
			CACHE_PTR(Z_CACHE_SLOT_P(function_name), NULL);
		} break;

		case ZEND_INIT_METHOD_CALL: {
			if (EX(opline)->op2_type == IS_CONST) {
				zval *function_name = EX_CONSTANT(EX(opline)->op2);
				CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(function_name), NULL, NULL);
			}
		} break;

		case ZEND_INIT_STATIC_METHOD_CALL: {
			zend_class_entry *ce;
			zval *mock;
			zend_string *key = NULL;
			
			if (EX(opline)->op1_type == IS_CONST) {
				key = zend_string_tolower(Z_STR_P(EX_CONSTANT(EX(opline)->op1)));
			} else if (EX(opline)->op1_type != IS_UNUSED) 	{
				ce = Z_CE_P(EX_VAR(EX(opline)->op1.var));
				if (!ce) {
					break;
				}
				key = zend_string_tolower(ce->name);
			}

			if (key && (mock = zend_hash_find(&UOPZ(mocks), key))) {
				zend_class_entry *poser;

				if (Z_TYPE_P(mock) == IS_STRING) {
					poser = zend_lookup_class(Z_STR_P(mock));
					if (!poser) {
						break;
					}
				} else poser = Z_OBJCE_P(mock);

				if (EX(opline)->op1_type == IS_CONST) {
					CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op1)), poser);
				} else {
					Z_CE_P(EX_VAR(EX(opline)->op1.var)) = poser;
				}
			}

			if (key && EX(opline)->op2_type == IS_CONST) {
				zval *function_name = EX_CONSTANT(EX(opline)->op2);
				if (EX(opline)->op1_type == IS_CONST) {
					CACHE_PTR(Z_CACHE_SLOT_P(function_name), NULL);
				} else {
					CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(function_name), NULL, NULL);
				}
			}
			
			if (key) {
				zend_string_release(key);
			}
		} break;
	}

	switch (EX(opline)->opcode) {
		case ZEND_INIT_FCALL_BY_NAME:
			if (uopz_init_fcall_by_name_handler)
				return uopz_init_fcall_by_name_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		case ZEND_INIT_FCALL:
			if (uopz_init_fcall_handler)
				return uopz_init_fcall_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		case ZEND_INIT_NS_FCALL_BY_NAME:
			if (uopz_init_ns_fcall_by_name_handler)
				return uopz_init_ns_fcall_by_name_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		case ZEND_INIT_METHOD_CALL:
			if (uopz_init_method_call_handler)
				return uopz_init_method_call_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		case ZEND_INIT_STATIC_METHOD_CALL:
			if (uopz_init_static_method_call_handler)
				return uopz_init_static_method_call_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

int uopz_constant_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
#if PHP_VERSION_ID >= 70100
	if (CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)))) {
		CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), NULL);
	}
#else
	if (EX(opline)->op1_type == IS_UNUSED) {
		if (CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)))) {
			CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), NULL);
		}
	} else {
		zend_string *key = NULL;
		zval *mock = NULL;
		zend_class_entry *poser = NULL;

		if (EX(opline)->op1_type == IS_CONST) {
			key = zend_string_tolower(Z_STR_P(EX_CONSTANT(EX(opline)->op1)));			

			if ((mock = zend_hash_find(&UOPZ(mocks), key))) {
				if (Z_TYPE_P(mock) == IS_OBJECT) {
					poser = Z_OBJCE_P(mock);
				} else poser = zend_lookup_class(Z_STR_P(mock));
				CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op1)), poser);
			}

			if (CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)))) {
				CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), NULL);
			}

			zend_string_release(key);		
		} else {
			key = zend_string_tolower(Z_CE_P(EX_VAR(EX(opline)->op1.var))->name);
			
			if ((mock = zend_hash_find(&UOPZ(mocks), key))) {
				if (Z_TYPE_P(mock) == IS_OBJECT) {
					poser = Z_OBJCE_P(mock);
				} else poser = zend_lookup_class(Z_STR_P(mock));

				Z_CE_P(EX_VAR(EX(opline)->op1.var)) = poser;
			}

			CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), 
								  Z_CE_P(EX_VAR(EX(opline)->op1.var)), NULL);

			zend_string_release(key);
		}
	}
#endif

	if (uopz_fetch_constant_handler) {
		return uopz_fetch_constant_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

int uopz_mock_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	int UOPZ_VM_ACTION = ZEND_USER_OPCODE_DISPATCH;
	zend_string *key;
	zval *mock = NULL;
	zend_class_entry *ce;

	if (EX(opline)->op1_type == IS_CONST) {
		ce = CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op1)));

		if (UNEXPECTED(ce == NULL)) {
			key = Z_STR_P(EX_CONSTANT(EX(opline)->op1));
		} else {
			key = ce->name;
		}

		key = zend_string_tolower(key);
	} else if(EX(opline)->op1_type == IS_UNUSED) {
		ce = zend_fetch_class(NULL, EX(opline)->op1.num);
		if (UNEXPECTED(ce == NULL)) {
			return UOPZ_VM_ACTION;
		}
		key = 
			zend_string_tolower(ce->name);
	} else {
		key = zend_string_tolower(
			Z_CE_P(EX_VAR(EX(opline)->op1.var))->name);
	}

	if (UNEXPECTED((mock = zend_hash_find(&UOPZ(mocks), key)))) {
		switch (Z_TYPE_P(mock)) {
			case IS_OBJECT:
				ZVAL_COPY(
					EX_VAR(EX(opline)->result.var), mock);
#if PHP_VERSION_ID < 70100
				EX(opline) = 
					OP_JMP_ADDR(EX(opline), EX(opline)->op2);
#else
				if (EX(opline)->extended_value == 0 && 
					(EX(opline)+1)->opcode == ZEND_DO_FCALL) {
					EX(opline) += 2;
				}
#endif
				UOPZ_VM_ACTION = ZEND_USER_OPCODE_CONTINUE;
			break;

			case IS_STRING:
				ce = zend_lookup_class(Z_STR_P(mock));
				if (EXPECTED(ce)) {
					if (EX(opline)->op1_type == IS_CONST) {
						CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op1)), ce);
					} else if (EX(opline)->op1_type != IS_UNUSED) {
						Z_CE_P(EX_VAR(EX(opline)->op1.var)) = ce;
					} else {
						/* oh dear, can't do what is requested */			
					}
					
				}
			break;
		}
	}

	zend_string_release(key);

	if (UOPZ_VM_ACTION == ZEND_USER_OPCODE_DISPATCH) {
		if (uopz_new_handler) {
			return uopz_new_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		}
	}

	return UOPZ_VM_ACTION;
} /* }}} */

static inline void uopz_run_hook(zend_function *function, zend_execute_data *execute_data) { /* {{{ */
	uopz_hook_t *uhook = uopz_find_hook(function);

	if (uhook && !uhook->busy) {
		uopz_execute_hook(uhook, execute_data);
	}
} /* }}} */

/* {{{ */
static inline int php_uopz_leave_helper(zend_execute_data *execute_data) {
	zend_execute_data *call = EX(call);

	EX(call) = call->prev_execute_data;
	EX(opline) = EX(opline) + 1;

	return ZEND_USER_OPCODE_LEAVE;
} /* }}} */

int uopz_return_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
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
					goto _uopz_return_handler_dispatch;
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

_uopz_return_handler_dispatch:
	if (uopz_do_fcall_handler) {
		return uopz_do_fcall_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

#ifdef ZEND_FETCH_CLASS_CONSTANT
int uopz_class_constant_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	if (EX(opline)->op1_type == IS_CONST) {
		zval *name = EX_CONSTANT(EX(opline)->op1);
		zend_string *key = Z_STR_P(name);
		zval *mock = NULL;
		zend_class_entry *poser = NULL;

		key = zend_string_tolower(key);
		
		if ((mock = zend_hash_find(&UOPZ(mocks), key))) {
			if (Z_TYPE_P(mock) == IS_OBJECT) {
				poser = Z_OBJCE_P(mock);
			} else poser = zend_lookup_class(Z_STR_P(mock));

			if (poser) {
				CACHE_PTR(Z_CACHE_SLOT_P(name), poser);
			}
		}

		zend_string_release(key);
	}

	CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), NULL);

	if (uopz_fetch_class_constant_handler) {
		return uopz_fetch_class_constant_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
	}
	
	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */
#endif

int uopz_fetch_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *name = NULL;
	zend_string *key = NULL;
	int UOPZ_VM_ACTION = ZEND_USER_OPCODE_DISPATCH;

	do {
		if (EX(opline)->op2_type == IS_UNUSED) {
			break;
		}
		
		if (EX(opline)->op2_type == IS_CONST) {
			name = 
				EX_CONSTANT(EX(opline)->op2);
			if (name) {
				key = Z_STR_P(name);
			}			
		} else if (EX(opline)->op2_type != IS_UNUSED) {
			name = EX_VAR(EX(opline)->op2.var);
			if (Z_TYPE_P(name) == IS_STRING) {
				key = Z_STR_P(name);
			} else if (Z_TYPE_P(name) == IS_OBJECT) {
				key = Z_OBJCE_P(name)->name;
			} else {
				
			}
		}

		if (key) {
			zval *mock = NULL;
			zend_class_entry *ce = NULL;
			zend_string *lookup = zend_string_tolower(key);

			if (UNEXPECTED((mock = zend_hash_find(&UOPZ(mocks), lookup)))) {

				switch (Z_TYPE_P(mock)) {
					case IS_OBJECT: ce = Z_OBJCE_P(mock); break;
					case IS_STRING: ce = zend_lookup_class(Z_STR_P(mock)); break;
				}

				if (ce) {
					if (EX(opline)->op2_type == IS_CONST) {
						CACHE_PTR(Z_CACHE_SLOT_P(name), ce);
					}

					Z_CE_P(EX_VAR(EX(opline)->result.var)) = ce;
					UOPZ_VM_ACTION = ZEND_USER_OPCODE_CONTINUE;
				}
			}

			zend_string_release(lookup);
		}
	} while(0);

	if (UOPZ_VM_ACTION == ZEND_USER_OPCODE_CONTINUE) {
		EX(opline) = EX(opline) + 1;
	} else {
		if (uopz_fetch_class_handler) {
			return uopz_fetch_class_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		}
	}

	return UOPZ_VM_ACTION;
} /* }}} */

int uopz_add_class_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	zval *name = EX_CONSTANT(EX(opline)->op2);
	zend_string *key = zend_string_tolower(Z_STR_P(name));
	zval *mock = NULL;
	
	if ((mock = zend_hash_find(&UOPZ(mocks), key))) {
		if (Z_TYPE_P(mock) == IS_STRING) {
			zend_class_entry *ce = zend_lookup_class(Z_STR_P(mock));
			
			if (ce) {
				CACHE_PTR(Z_CACHE_SLOT_P(name), ce);
			}
		} else {
			CACHE_PTR(Z_CACHE_SLOT_P(name), Z_OBJCE_P(mock));
		}
	}
	
	zend_string_release(key);	

	if (uopz_add_trait_handler || uopz_add_interface_handler) {
		switch (EX(opline)->opcode) {
			case ZEND_ADD_INTERFACE:
				return uopz_add_interface_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);

			case ZEND_ADD_TRAIT:
				return uopz_add_trait_handler(UOPZ_OPCODE_HANDLER_ARGS_PASSTHRU);
		}
	}
	
	return ZEND_USER_OPCODE_DISPATCH;
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
