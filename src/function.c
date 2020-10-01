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

#ifndef UOPZ_FUNCTION
#define UOPZ_FUNCTION

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "function.h"
#include "copy.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

zend_bool uopz_add_function(zend_class_entry *clazz, zend_string *name, zval *closure, zend_long flags, zend_bool all) { /* {{{ */
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_string *key = zend_string_tolower(name);
	zend_function *function = NULL;
	HashTable *functions = NULL;

	if (zend_hash_exists(table, key)) {
		if (clazz) {
			uopz_exception(
				"will not replace existing method %s::%s, use uopz_set_return instead",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
		} else {
			uopz_exception(
				"will not replace existing function %s, use uopz_set_return instead",
				ZSTR_VAL(name));
		}
		zend_string_release(key);
		return 0;
	}

	if (!(functions = zend_hash_index_find_ptr(&UOPZ(functions), (zend_long) table))) {
		ALLOC_HASHTABLE(functions);
		zend_hash_init(functions, 8, NULL, uopz_zval_dtor, 0);
		zend_hash_index_update_ptr(
			&UOPZ(functions), (zend_long) table, functions);
	}

	zend_hash_update(functions, key, closure);
	zval_copy_ctor(closure);
#if PHP_VERSION_ID >= 80000
	function = uopz_copy_closure(clazz, 
			(zend_function*) zend_get_closure_method_def(Z_OBJ_P(closure)),
			flags);
#else
	function = uopz_copy_closure(clazz, 
			(zend_function*) zend_get_closure_method_def(closure),
			flags);
#endif
	zend_hash_update_ptr(table, key, (void*) function);

	if (clazz) {
		if (all) {
			zend_class_entry *next;

			ZEND_HASH_FOREACH_PTR(CG(class_table), next) {
				if (next->parent == clazz) {
					if (zend_hash_exists(&next->function_table, key)) {
						continue;
					}
					uopz_add_function(next, name, closure, flags, all);
				}
			} ZEND_HASH_FOREACH_END();
		}

		uopz_handle_magic(clazz, name, function);
	}

	zend_string_release(key);

	return 1;
} /* }}} */

zend_bool uopz_del_function(zend_class_entry *clazz, zend_string *name, zend_bool all) { /* {{{ */
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	HashTable *functions = (HashTable*) 
		zend_hash_index_find_ptr(&UOPZ(functions), (zend_long) table);	
	zend_string *key = zend_string_tolower(name);

	if (!functions || !zend_hash_exists(functions, key)) {
		if (clazz) {
			uopz_exception(
				"cannot delete method %s::%s, it was not added by uopz",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
		} else {
			uopz_exception(
				"cannot delete function %s, it was not added by uopz",
				ZSTR_VAL(name));
		}
		zend_string_release(key);
		return 0;
	}

	if (clazz) {
		if (all) {
			zend_class_entry *next;

			ZEND_HASH_FOREACH_PTR(CG(class_table), next) {
				if (next->parent == clazz) {
					if (!zend_hash_exists(&next->function_table, key)) {
						continue;
					}
					uopz_del_function(next, name, all);
				}
			} ZEND_HASH_FOREACH_END();
		}
	}

	zend_hash_del(table, key);
	zend_hash_del(functions, key);
	zend_string_release(key);
	
	return 1;
} /* }}} */

/* {{{ */
void uopz_flags(zend_class_entry *clazz, zend_string *name, zend_long flags, zval *return_value) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_function *function = NULL;
	zend_long current = 0;

	if (!name || !ZSTR_LEN(name) || !ZSTR_VAL(name)) {
		if (flags == ZEND_LONG_MAX) {
			RETURN_LONG(clazz->ce_flags);
		}

		if (flags & ZEND_ACC_PPP_MASK) {
			uopz_exception(
				"attempt to set public, private or protected on class entry %s, not allowed",
				ZSTR_VAL(clazz->name));
			return;
		}

		if (flags & ZEND_ACC_STATIC) {
			uopz_exception(
				"attempt to set static on class entry %s, not allowed",
				ZSTR_VAL(clazz->name));
			return;
		}

#if PHP_VERSION_ID >= 70400
        if (clazz->ce_flags & ZEND_ACC_IMMUTABLE) {
			uopz_exception(
				"attempt to set flags of immutable class entry %s, not allowed",
				ZSTR_VAL(clazz->name));
			return;
        }
#endif

		current = clazz->ce_flags;
		clazz->ce_flags = flags;
#if PHP_VERSION_ID >= 70400
        if (current & ZEND_ACC_LINKED) {
            clazz->ce_flags |= ZEND_ACC_LINKED;
        }
#endif
		RETURN_LONG(current);
	}

	if (uopz_find_function(table, name, &function) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"failed to set or get flags of method %s::%s, it does not exist",
				ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		} else {
			uopz_exception(
				"failed to set or get flags of function %s, it does not exist",
				ZSTR_VAL(name));
		}
		return;
	}

	if (flags == ZEND_LONG_MAX) {
		RETURN_LONG(function->common.fn_flags);
	}

	current = function->common.fn_flags;
	if (flags) {
#if PHP_VERSION_ID >= 70400
        if (function->common.fn_flags & ZEND_ACC_IMMUTABLE) {
            uopz_exception(
				"attempt to set flags of immutable function entry %s, not allowed",
				ZSTR_VAL(name));
        }
#endif
		function->common.fn_flags = flags;
	}
	RETURN_LONG(current);
} /* }}} */

static inline void uopz_try_addref(zval *z) { /* {{{ */
	Z_TRY_ADDREF_P(z);
} /* }}} */

zend_bool uopz_set_static(zend_class_entry *clazz, zend_string *function, zval *statics) { /* {{{ */
	zend_function *entry;
	zend_string *k = NULL;
	zval *v = NULL;

	if (clazz) {
		if (uopz_find_function(&clazz->function_table, function, &entry) != SUCCESS) {
			uopz_exception(
				"failed to set statics in method %s::%s, it does not exist",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
			return 0;
		}
	} else {
		if (uopz_find_function(CG(function_table), function, &entry) != SUCCESS) {
			uopz_exception(
				"failed to set statics in function %s, it does not exist",
				ZSTR_VAL(function));
			return 0;
		}
	}

	if (entry->type != ZEND_USER_FUNCTION) {
		if (clazz) {
			uopz_exception(
				"failed to set statics in internal method %s::%s",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
		} else {
			uopz_exception(
				"failed to set statics in internal function %s",
				ZSTR_VAL(function));
		}

		return 0;
	}

	if (!entry->op_array.static_variables) {
		if (clazz) {
			uopz_exception(
				"failed to set statics in method %s::%s, no statics declared",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
		} else {
			uopz_exception(
				"failed to set statics in function %s, no statics declared",
				ZSTR_VAL(function));
		}

		return 0;
	}

	ZEND_HASH_FOREACH_STR_KEY_VAL(entry->op_array.static_variables, k, v) {
		zval *y;

		if (Z_REFCOUNTED_P(v)) {
			zval_ptr_dtor(v);
		}

		if (!(y = zend_hash_find(Z_ARRVAL_P(statics), k))) {
			ZVAL_NULL(v);
			
			continue;
		}
		
		ZVAL_COPY(v, y);
	} ZEND_HASH_FOREACH_END();

	return 1;
} /* }}} */

zend_bool uopz_get_static(zend_class_entry *clazz, zend_string *function, zval *return_value) { /* {{{ */
	zend_function *entry;

	if (clazz) {
		if (uopz_find_function(&clazz->function_table, function, &entry) != SUCCESS) {
			uopz_exception(
				"failed to get statics from method %s::%s, it does not exist",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
			return 0;
		}
	} else {
		if (uopz_find_function(CG(function_table), function, &entry) != SUCCESS) {
			uopz_exception(
				"failed to get statics from function %s, it does not exist",
				ZSTR_VAL(function));
			return 0;
		}
	}

	if (entry->type != ZEND_USER_FUNCTION) {
		if (clazz) {
			uopz_exception(
				"failed to get statics from internal method %s::%s",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
		} else {
			uopz_exception(
				"failed to get statics from internal function %s",
				ZSTR_VAL(function));
		}

		return 0;
	}

	if (!entry->op_array.static_variables) {
		if (clazz) {
			uopz_exception(
				"failed to set statics in method %s::%s, no statics declared",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
		} else {
			uopz_exception(
				"failed to set statics in function %s, no statics declared",
				ZSTR_VAL(function));
		}

		return 0;
	}

	ZVAL_ARR(return_value, entry->op_array.static_variables);
#if PHP_VERSION_ID >= 70300
	GC_ADDREF(entry->op_array.static_variables);
#else
	GC_REFCOUNT(entry->op_array.static_variables)++;
#endif
	return 1;
} /* }}} */

#endif	/* UOPZ_FUNCTION */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
