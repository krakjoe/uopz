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

#ifndef UOPZ_HOOK
#define UOPZ_HOOK

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "hook.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

zend_bool uopz_set_hook(zend_class_entry *clazz, zend_string *name, zval *closure) { /* {{{ */
	HashTable *hooks;
	uopz_hook_t hook;
	zend_string *key = zend_string_tolower(name);

	if (clazz && uopz_find_function(&clazz->function_table, key, NULL) != SUCCESS) {
		uopz_exception(
			"failed to set hook for %s::%s, the method does not exist",
			ZSTR_VAL(clazz->name),
			ZSTR_VAL(name));
		zend_string_release(key);
		return 0;
	}

	if (clazz) {
		hooks = zend_hash_find_ptr(&UOPZ(hooks), clazz->name);
	} else hooks = zend_hash_index_find_ptr(&UOPZ(hooks), 0);
	
	if (!hooks) {
		ALLOC_HASHTABLE(hooks);
		zend_hash_init(hooks, 8, NULL, uopz_hook_free, 0);
		if (clazz) {
			zend_hash_update_ptr(&UOPZ(hooks), clazz->name, hooks);
		} else zend_hash_index_update_ptr(&UOPZ(hooks), 0, hooks);
	}

	memset(&hook, 0, sizeof(uopz_hook_t));
	
	hook.clazz = clazz;
	hook.function = zend_string_copy(name);
	ZVAL_COPY(&hook.closure, closure);

	zend_hash_update_mem(hooks, key, &hook, sizeof(uopz_hook_t));
	zend_string_release(key);

	if (clazz && clazz->parent) {
		if (uopz_find_method(clazz->parent, name, NULL) == SUCCESS) {
			return uopz_set_hook(clazz->parent, name, closure);	
		}
	}
	
	return 1;
} /* }}} */

zend_bool uopz_unset_hook(zend_class_entry *clazz, zend_string *function) { /* {{{ */
	HashTable *hooks;
	zend_string *key = zend_string_tolower(function);

	if (clazz) {
		hooks = zend_hash_find_ptr(&UOPZ(hooks), clazz->name);
	} else hooks = zend_hash_index_find_ptr(&UOPZ(hooks), 0);

	if (!hooks || !zend_hash_exists(hooks, key)) {
		zend_string_release(key);
		return 0;
	}

	zend_hash_del(hooks, key);
	zend_string_release(key);

	return 1;
} /* }}} */

void uopz_get_hook(zend_class_entry *clazz, zend_string *function, zval *return_value) { /* {{{ */
	HashTable *hooks;
	uopz_hook_t *uhook;
	
	if (clazz) {
		hooks = zend_hash_find_ptr(&UOPZ(hooks), clazz->name);
	} else hooks = zend_hash_index_find_ptr(&UOPZ(hooks), 0);

	if (!hooks) {
		return;
	}

	uhook = zend_hash_find_ptr(hooks, function);

	if (!uhook) {
		return;
	}

	ZVAL_COPY(return_value, &uhook->closure);
} /* }}} */

uopz_hook_t* uopz_find_hook(zend_function *function) { /* {{{ */
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

void uopz_execute_hook(uopz_hook_t *uhook, zend_execute_data *execute_data) { /* {{{ */
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	char *error = NULL;
	zval closure, rv;

	ZVAL_UNDEF(&rv);

	uhook->busy = 1;

	zend_create_closure(&closure, (zend_function*) zend_get_closure_method_def(&uhook->closure), 
		uhook->clazz, uhook->clazz, Z_OBJ(EX(This)) ? &EX(This) : NULL);

	if (zend_fcall_info_init(&closure, 0, &fci, &fcc, NULL, &error) != SUCCESS) {
		if (EX(func)->common.scope) {
			uopz_exception("cannot use hook set for %s::%s as function: %s",
				ZSTR_VAL(EX(func)->common.scope->name),
				ZSTR_VAL(EX(func)->common.function_name), error);
		} else {
			uopz_exception("cannot use hook set for %s as function: %s",
				ZSTR_VAL(EX(func)->common.function_name), error);
		}		
		
		if (error) {
			efree(error);
		}
		goto _exit_uopz_execute_hook;
	}

	if (zend_fcall_info_argp(&fci, EX_NUM_ARGS(), EX_VAR_NUM(0)) != SUCCESS) {
		if (EX(func)->common.scope) {
			uopz_exception("cannot set arguments for %s::%s hook",
				ZSTR_VAL(EX(func)->common.scope->name),
				ZSTR_VAL(EX(func)->common.function_name));
		} else {
			uopz_exception("cannot set arguments for %s hook",
			ZSTR_VAL(EX(func)->common.function_name));
		}
		
		goto _exit_uopz_execute_hook;
	}

	fci.retval= &rv;
	
	if (zend_call_function(&fci, &fcc) == SUCCESS) {
		zend_fcall_info_args_clear(&fci, 1);
		if (!Z_ISUNDEF(rv)) {
			zval_ptr_dtor(&rv);
		}
	}

_exit_uopz_execute_hook:
	zval_ptr_dtor(&closure);

	uhook->busy = 0;
} /* }}} */

void uopz_hook_free(zval *zv) { /* {{{ */
	uopz_hook_t *uhook = Z_PTR_P(zv);
	
	zend_string_release(uhook->function);
	zval_ptr_dtor(&uhook->closure);
	efree(uhook);
} /* }}} */
#endif	/* UOPZ_HOOK */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
