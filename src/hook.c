/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2016-2020                                  |
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
	zend_function *function;

	if (clazz) {
		if (uopz_find_method(clazz, key, &function) != SUCCESS) {
			uopz_exception(
				"failed to set hook for %s::%s, the method does not exist",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
			zend_string_release(key);
			return 0;
		}

		if (function->common.scope != clazz) {
			uopz_exception(
				"failed to set hook for %s::%s, the method is defined in %s",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name),
				ZSTR_VAL(function->common.scope->name));
			zend_string_release(key);
			return 0;
		}
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

	zend_hash_update_mem(
		hooks, key, &hook, sizeof(uopz_hook_t));
	zend_string_release(key);
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
	zend_string *key = zend_string_tolower(function);
	
	if (clazz) {
		hooks = zend_hash_find_ptr(&UOPZ(hooks), clazz->name);
	} else hooks = zend_hash_index_find_ptr(&UOPZ(hooks), 0);

	if (!hooks || !zend_hash_exists(hooks, key)) {
		zend_string_release(key);
		return;
	}

	uhook = zend_hash_find_ptr(hooks, key);

	ZVAL_COPY(return_value, &uhook->closure);

	zend_string_release(key);
} /* }}} */

uopz_hook_t* uopz_find_hook(zend_function *function) { /* {{{ */
	zend_string *key;
	uopz_hook_t *uhook;
	HashTable *hooks;

	if (!function) {
		return NULL;
	}

	if (!function->common.function_name) {
		return NULL;
	}

	if (function->common.scope) {
		hooks = zend_hash_find_ptr(&UOPZ(hooks), function->common.scope->name);
	} else {
		hooks = zend_hash_index_find_ptr(&UOPZ(hooks), 0);
	}

	if (!hooks) {
		if (function->common.prototype && 
		    function->common.prototype->common.scope &&
		    function->common.prototype->common.scope->ce_flags & ZEND_ACC_INTERFACE) {
			return uopz_find_hook(
				function->common.prototype);
		}
		return NULL;
	}

	key = zend_string_tolower(function->common.function_name);
	uhook = zend_hash_find_ptr(hooks, key);
	zend_string_release(key);

	return uhook;
} /* }}} */

void uopz_execute_hook(uopz_hook_t *uhook, zend_execute_data *execute_data, zend_bool skip, zend_bool variadic) { /* {{{ */
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	char *error = NULL;
	zval closure, rv;

	ZVAL_UNDEF(&rv);

	uhook->busy = 1;

#if PHP_VERSION_ID >= 80000
	zend_create_closure(&closure, (zend_function*) zend_get_closure_method_def(Z_OBJ(uhook->closure)), 
#else
	zend_create_closure(&closure, (zend_function*) zend_get_closure_method_def(&uhook->closure), 
#endif
		uhook->clazz, uhook->clazz, Z_OBJ(EX(This)) ? &EX(This) : NULL);

	zend_fcall_info_init(&closure, 0, &fci, &fcc, NULL, &error);

	if (!skip) {
        fci.param_count = ZEND_CALL_NUM_ARGS(execute_data);
	    fci.params = ZEND_CALL_ARG(execute_data, 1);
    } else {
        if (variadic) {
            zend_fcall_info_args_ex(
                &fci,
                fcc.function_handler,
                ZEND_CALL_ARG(execute_data, 2));
        } else {
            fci.param_count = ZEND_CALL_NUM_ARGS(execute_data) - 1;
	        fci.params = ZEND_CALL_ARG(execute_data, 2);
        }
    }

	fci.retval= &rv;
	
	if (zend_call_function(&fci, &fcc) == SUCCESS) {
		if (!Z_ISUNDEF(rv)) {
			zval_ptr_dtor(&rv);
		}
	}

    if (variadic) {
        zend_fcall_info_args_clear(&fci, 1);
    }

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
