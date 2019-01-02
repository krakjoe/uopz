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

#ifndef UOPZ_RETURN
#define UOPZ_RETURN

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "return.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

zend_bool uopz_set_return(zend_class_entry *clazz, zend_string *name, zval *value, zend_bool execute) { /* {{{ */
	HashTable *returns;
	uopz_return_t ret;
	zend_string *key = zend_string_tolower(name);
	zend_function *function;

	if (clazz) {
		if (uopz_find_method(clazz, key, &function) != SUCCESS) {
			uopz_exception(
				"failed to set return for %s::%s, the method does not exist",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
			zend_string_release(key);
			return 0;
		}

		if (function->common.scope != clazz) {
			uopz_exception(
				"failed to set return for %s::%s, the method is defined in %s",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name),
				ZSTR_VAL(function->common.scope->name));
			zend_string_release(key);
			return 0;
		}
	}

	if (clazz) {
		returns = zend_hash_find_ptr(&UOPZ(returns), clazz->name);
	} else returns = zend_hash_index_find_ptr(&UOPZ(returns), 0);
	
	if (!returns) {
		ALLOC_HASHTABLE(returns);
		zend_hash_init(returns, 8, NULL, uopz_return_free, 0);
		if (clazz) {
			zend_hash_update_ptr(&UOPZ(returns), clazz->name, returns);
		} else zend_hash_index_update_ptr(&UOPZ(returns), 0, returns);
	}

	memset(&ret, 0, sizeof(uopz_return_t));
	
	ret.clazz = clazz;
	ret.function = zend_string_copy(name);
	ZVAL_COPY(&ret.value, value);
	ret.flags = execute ? UOPZ_RETURN_EXECUTE : 0;

	zend_hash_update_mem(returns, key, &ret, sizeof(uopz_return_t));

	zend_string_release(key);
	return 1;
} /* }}} */

zend_bool uopz_unset_return(zend_class_entry *clazz, zend_string *function) { /* {{{ */
	HashTable *returns;
	zend_string *key = zend_string_tolower(function);
	
	if (clazz) {
		returns = zend_hash_find_ptr(&UOPZ(returns), clazz->name);
	} else returns = zend_hash_index_find_ptr(&UOPZ(returns), 0);

	if (!returns || !zend_hash_exists(returns, key)) {
		zend_string_release(key);
		return 0;
	}

	zend_hash_del(returns, key);
	zend_string_release(key);

	return 1;
} /* }}} */

void uopz_get_return(zend_class_entry *clazz, zend_string *function, zval *return_value) { /* {{{ */
	HashTable *returns;
	uopz_return_t *ureturn;

	if (clazz) {
		returns = zend_hash_find_ptr(&UOPZ(returns), clazz->name);
	} else returns = zend_hash_index_find_ptr(&UOPZ(returns), 0);

	if (!returns) {
		return;
	}

	ureturn = zend_hash_find_ptr(returns, function);

	if (!ureturn) {
		return;
	}
	
	ZVAL_COPY(return_value, &ureturn->value);
} /* }}} */

uopz_return_t* uopz_find_return(zend_function *function) { /* {{{ */
	zend_string *key;
	uopz_return_t *ureturn;
	HashTable *returns;

	if (function->common.fn_flags & ZEND_ACC_CLOSURE) {
		return NULL;
	}

	if (!function->common.function_name) {
		return NULL;
	}

	if (function->common.scope) {
		returns = zend_hash_find_ptr(&UOPZ(returns), function->common.scope->name);
	} else {
		returns = zend_hash_index_find_ptr(&UOPZ(returns), 0);
	}

	if (!returns) {
		if (function->common.prototype) {
			return uopz_find_return(
				function->common.prototype);
		}

		return NULL;
	}

	key = zend_string_tolower(function->common.function_name);
	ureturn = zend_hash_find_ptr(returns, key);
	zend_string_release(key);

	return ureturn;
} /* }}} */

extern PHP_FUNCTION(php_call_user_func);

void uopz_execute_return(uopz_return_t *ureturn, zend_execute_data *execute_data, zval *return_value) { /* {{{ */
	zend_fcall_info fci = empty_fcall_info;
	zend_fcall_info_cache fcc = empty_fcall_info_cache;
	char *error = NULL;
	zval closure, 
		 rv,
		 *result = return_value ? return_value : &rv;

	ZVAL_UNDEF(&rv);

	ureturn->flags ^= UOPZ_RETURN_BUSY;

	zend_create_closure(&closure, (zend_function*) zend_get_closure_method_def(&ureturn->value), 
		ureturn->clazz, ureturn->clazz, Z_OBJ(EX(This)) ? &EX(This) : NULL);

	zend_fcall_info_init(&closure, 0, &fci, &fcc, NULL, &error);

	if (uopz_is_cuf(execute_data)) {
		fci.params = ZEND_CALL_ARG(execute_data, 2);
		fci.param_count = ZEND_CALL_NUM_ARGS(execute_data) - 1;
	} else if (uopz_is_cufa(execute_data)) {
		zend_fcall_info_args(&fci, ZEND_CALL_ARG(execute_data, 2));
	} else {
		fci.params = ZEND_CALL_ARG(execute_data, 1);
		fci.param_count = ZEND_CALL_NUM_ARGS(execute_data);
	}

	fci.retval = result;

	if (zend_call_function(&fci, &fcc) == SUCCESS) {
		if (!return_value) {
			if (!Z_ISUNDEF(rv)) {
				zval_ptr_dtor(&rv);
			}
		}
	}

	zval_ptr_dtor(&closure);

	ureturn->flags ^= UOPZ_RETURN_BUSY;
} /* }}} */

void uopz_return_free(zval *zv) { /* {{{ */
	uopz_return_t *ureturn = Z_PTR_P(zv);
	
	zend_string_release(ureturn->function);
	zval_ptr_dtor(&ureturn->value);
	efree(ureturn);
} /* }}} */

#endif	/* UOPZ_RETURN */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
