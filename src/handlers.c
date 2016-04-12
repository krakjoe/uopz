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
#else
#	define UOPZ_OPCODE_HANDLER_ARGS zend_execute_data *execute_data
#endif

ZEND_EXTERN_MODULE_GLOBALS(uopz);

int uopz_call_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_constant_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_mock_handler(UOPZ_OPCODE_HANDLER_ARGS);
int uopz_return_handler(UOPZ_OPCODE_HANDLER_ARGS);

void uopz_handlers_init(void) {
	zend_set_user_opcode_handler(ZEND_INIT_FCALL_BY_NAME,		uopz_call_handler);
	zend_set_user_opcode_handler(ZEND_INIT_FCALL,				uopz_call_handler);
	zend_set_user_opcode_handler(ZEND_INIT_NS_FCALL_BY_NAME,	uopz_call_handler);
	zend_set_user_opcode_handler(ZEND_INIT_METHOD_CALL,			uopz_call_handler);
	zend_set_user_opcode_handler(ZEND_INIT_STATIC_METHOD_CALL,	uopz_call_handler);
	zend_set_user_opcode_handler(ZEND_NEW,						uopz_mock_handler);
	zend_set_user_opcode_handler(ZEND_FETCH_CONSTANT,			uopz_constant_handler);
	zend_set_user_opcode_handler(ZEND_DO_FCALL,					uopz_return_handler);
}

void uopz_handlers_shutdown(void) {
	zend_set_user_opcode_handler(ZEND_INIT_FCALL_BY_NAME,		NULL);
	zend_set_user_opcode_handler(ZEND_INIT_FCALL,				NULL);
	zend_set_user_opcode_handler(ZEND_INIT_NS_FCALL_BY_NAME,	NULL);
	zend_set_user_opcode_handler(ZEND_INIT_METHOD_CALL,			NULL);
	zend_set_user_opcode_handler(ZEND_INIT_STATIC_METHOD_CALL,	NULL);
	zend_set_user_opcode_handler(ZEND_NEW,						NULL);
	zend_set_user_opcode_handler(ZEND_FETCH_CONSTANT,			NULL);
	zend_set_user_opcode_handler(ZEND_DO_FCALL,					NULL);
}

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
			if (EX(opline)->op2_type == IS_CONST) {
				zval *function_name = EX_CONSTANT(EX(opline)->op2);
				if (EX(opline)->op1_type == IS_CONST) {
					CACHE_PTR(Z_CACHE_SLOT_P(function_name), NULL);
				} else {
					CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(function_name), NULL, NULL);
				}
			}
		} break;
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
		if (EX(opline)->op1_type == IS_CONST) {
			if (CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)))) {
				CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), NULL);
			}
		} else {
			CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op2)), 
								  Z_CE_P(EX_VAR(EX(opline)->op1.var)), NULL);
		}
	}
#endif

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

int uopz_mock_handler(UOPZ_OPCODE_HANDLER_ARGS) { /* {{{ */
	int UOPZ_VM_ACTION = ZEND_USER_OPCODE_DISPATCH;

	if (EXPECTED(EX(opline)->op1_type == IS_CONST)) {
		zend_string *key;
		zend_string *clazz = NULL;
		zval *mock = NULL;
		zend_class_entry *ce = CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op1)));

		if (UNEXPECTED(ce == NULL)) {
			clazz = Z_STR_P(EX_CONSTANT(EX(opline)->op1));
		} else {
			clazz = ce->name;
		}

		key = zend_string_tolower(clazz);

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
					if (EXPECTED(ce))	 {
						CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(EX(opline)->op1)), ce);
					}
				break;
			}
		}

		zend_string_release(key);
	}

	return UOPZ_VM_ACTION;
} /* }}} */

static inline void uopz_run_hook(zend_function *function, zend_execute_data *execute_data) { /* {{{ */
	uopz_hook_t *uhook = uopz_find_hook(function);

	if (uhook && !uhook->busy) {
		uopz_execute_hook(uhook, execute_data);
	}
} /* }}} */

int uopz_return_handler(UOPZ_OPCODE_HANDLER_ARGS) {
	zend_execute_data *call = EX(call);

	if (call) {
		uopz_return_t *ureturn;

		uopz_run_hook(call->func, call);

		ureturn = uopz_find_return(call->func);

		if (ureturn) {
			zval *return_value = EX_VAR(EX(opline)->result.var);

			if (UOPZ_RETURN_IS_EXECUTABLE(ureturn)) {
				if (UOPZ_RETURN_IS_BUSY(ureturn)) {
					return ZEND_USER_OPCODE_DISPATCH;
				}

				uopz_execute_return(ureturn, call, return_value);

				EX(call) = call->prev_execute_data;
				EX(opline)++;
				
				return ZEND_USER_OPCODE_CONTINUE;
			}

			if (return_value) {
				ZVAL_COPY(return_value, &ureturn->value);
			}
			
			EX(call) = call->prev_execute_data;
			EX(opline)++;

			return ZEND_USER_OPCODE_CONTINUE;
		}
	}

	return ZEND_USER_OPCODE_DISPATCH;
}
#endif	/* UOPZ_HANDLERS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
