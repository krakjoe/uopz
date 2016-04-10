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

#ifndef UOPZ_EXECUTORS
#define UOPZ_EXECUTORS

#include "php.h"
#include "uopz.h"
#include "executors.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

zend_execute_internal_f zend_execute_internal_function;
zend_execute_f zend_execute_function;

static inline uopz_hook_t* uopz_find_hook(zend_function *function) { /* {{{ */
	HashTable *returns = function->common.scope ? zend_hash_find_ptr(&UOPZ(hooks), function->common.scope->name) :
												  zend_hash_index_find_ptr(&UOPZ(hooks), 0);

	if (returns && function->common.function_name) {
		Bucket *bucket;

		ZEND_HASH_FOREACH_BUCKET(returns, bucket) {
			if (zend_string_equals_ci(function->common.function_name, bucket->key)) {
				return Z_PTR(bucket->val);
			}
		} ZEND_HASH_FOREACH_END();
	}

	return NULL;
} /* }}} */

static inline uopz_return_t* uopz_find_return(zend_function *function) { /* {{{ */
	HashTable *returns = function->common.scope ? zend_hash_find_ptr(&UOPZ(returns), function->common.scope->name) :
												  zend_hash_index_find_ptr(&UOPZ(returns), 0);

	if (returns && function->common.function_name) {
		Bucket *bucket;

		ZEND_HASH_FOREACH_BUCKET(returns, bucket) {
			if (zend_string_equals_ci(function->common.function_name, bucket->key)) {
				return Z_PTR(bucket->val);
			}
		} ZEND_HASH_FOREACH_END();
	}

	return NULL;
} /* }}} */

static inline void php_uopz_execute_return(uopz_return_t *ureturn, zend_execute_data *execute_data, zval *return_value) { /* {{{ */
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	char *error = NULL;
	zval closure, 
		 rv,
		 *result = return_value ? return_value : &rv;
	const zend_function *overload = zend_get_closure_method_def(&ureturn->value);

	zend_execute_data *prev_execute_data = execute_data;

	ZVAL_UNDEF(&rv);

	ureturn->flags ^= UOPZ_RETURN_BUSY;

	zend_create_closure(&closure, (zend_function*) overload, 
		ureturn->clazz, ureturn->clazz, Z_OBJ(EX(This)) ? &EX(This) : NULL);

	if (zend_fcall_info_init(&closure, 0, &fci, &fcc, NULL, &error) != SUCCESS) {
		uopz_exception("cannot use return value set for %s as function: %s",
			ZSTR_VAL(EX(func)->common.function_name), error);
		if (error) {
			efree(error);
		}
		goto _exit_php_uopz_execute_return;
	}

	if (zend_fcall_info_argp(&fci, EX_NUM_ARGS(), EX_VAR_NUM(0)) != SUCCESS) {
		uopz_exception("cannot set arguments for %s",
			ZSTR_VAL(EX(func)->common.function_name));
		goto _exit_php_uopz_execute_return;
	}

	fci.retval= result;
	
	if (zend_call_function(&fci, &fcc) == SUCCESS) {
		zend_fcall_info_args_clear(&fci, 1);

		if (!return_value) {
			if (!Z_ISUNDEF(rv)) {
				zval_ptr_dtor(&rv);
			}
		}
	}

_exit_php_uopz_execute_return:
	zval_ptr_dtor(&closure);

	ureturn->flags ^= UOPZ_RETURN_BUSY;

	EG(current_execute_data) = prev_execute_data;
} /* }}} */

static inline void php_uopz_execute_hook(uopz_hook_t *uhook, zend_execute_data *execute_data) { /* {{{ */
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	char *error = NULL;
	zval closure, rv;
	const zend_function *overload = zend_get_closure_method_def(&uhook->closure);

	zend_execute_data *prev_execute_data = execute_data;

	ZVAL_UNDEF(&rv);

	uhook->busy = 1;

	zend_create_closure(&closure, (zend_function*) overload, 
		uhook->clazz, uhook->clazz, Z_OBJ(EX(This)) ? &EX(This) : NULL);

	if (zend_fcall_info_init(&closure, 0, &fci, &fcc, NULL, &error) != SUCCESS) {
		uopz_exception("cannot use hook set for %s as function: %s",
			ZSTR_VAL(EX(func)->common.function_name), error);
		if (error) {
			efree(error);
		}
		goto _exit_php_uopz_execute_hook;
	}

	if (zend_fcall_info_argp(&fci, EX_NUM_ARGS(), EX_VAR_NUM(0)) != SUCCESS) {
		uopz_exception("cannot set arguments for %s hook",
			ZSTR_VAL(EX(func)->common.function_name));
		goto _exit_php_uopz_execute_hook;
	}

	fci.retval= &rv;
	
	if (zend_call_function(&fci, &fcc) == SUCCESS) {
		zend_fcall_info_args_clear(&fci, 1);
		if (!Z_ISUNDEF(rv)) {
			zval_ptr_dtor(&rv);
		}
	}

_exit_php_uopz_execute_hook:
	zval_ptr_dtor(&closure);

	uhook->busy = 0;

	EG(current_execute_data) = prev_execute_data;
} /* }}} */

static inline void uopz_run_hooks(zend_execute_data *execute_data) { /* {{{ */
	if (EX(func)) {
		uopz_hook_t *uhook = uopz_find_hook(EX(func));

		if (uhook && !uhook->busy) {
			php_uopz_execute_hook(uhook, execute_data);
		}
	}
} /* }}} */

void php_uopz_execute_internal(zend_execute_data *execute_data, zval *return_value) { /* {{{ */
	uopz_run_hooks(execute_data);

	if (EX(func) ) {
		uopz_return_t *ureturn = uopz_find_return(EX(func));

		if (ureturn) {
			if (UOPZ_RETURN_IS_EXECUTABLE(ureturn)) {
				if (UOPZ_RETURN_IS_BUSY(ureturn)) {
					goto _php_uopz_execute_internal;
				}

				php_uopz_execute_return(ureturn, execute_data, return_value);
				return;
			}

			if (return_value) {
				ZVAL_COPY(return_value, &ureturn->value);
			}
			return;
		}
	}

_php_uopz_execute_internal:
	if (zend_execute_internal_function) {
		zend_execute_internal_function(execute_data, return_value);
	} else execute_internal(execute_data, return_value);
} /* }}} */

void php_uopz_execute(zend_execute_data *execute_data) { /* {{{ */
	uopz_run_hooks(execute_data);

	if (EX(func)) {
		uopz_return_t *ureturn = uopz_find_return(EX(func));

		if (ureturn) {
			if (UOPZ_RETURN_IS_EXECUTABLE(ureturn)) {
				if (UOPZ_RETURN_IS_BUSY(ureturn)) {
					goto _php_uopz_execute;
				}

				php_uopz_execute_return(ureturn, execute_data, EX(return_value));
				return;
			}

			if (EX(return_value)) {
				ZVAL_COPY(EX(return_value), &ureturn->value);
			}
			return;
		}
	}

_php_uopz_execute:
	if (zend_execute_function) {
		zend_execute_function(execute_data);
	} else execute_ex(execute_data);
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
