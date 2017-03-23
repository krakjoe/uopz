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

#ifndef UOPZ_FUNCTION
#define UOPZ_FUNCTION

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "function.h"
#include "copy.h"

#include <Zend/zend_closures.h>
#include <Zend/zend_exceptions.h>

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

	if (!(functions = zend_hash_index_find_ptr(&UOPZ(functions), (zend_ulong) table))) {
		ALLOC_HASHTABLE(functions);
		zend_hash_init(functions, 8, NULL, uopz_zval_dtor, 0);
		zend_hash_index_update_ptr(
			&UOPZ(functions), (zend_long) table, functions);
	}

	if (!zend_hash_update(functions, key, closure)) {
		if (clazz) {
			uopz_exception(
				"failed to update uopz function table while adding method %s::%s",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
		} else {
			uopz_exception(
				"failed to update uopz function table while adding function %s",
				ZSTR_VAL(name));
		}
		zend_string_release(key);
		return 0;
	}

	zval_copy_ctor(closure);

	function = uopz_copy_closure(clazz, (zend_function*) zend_get_closure_method_def(closure), flags);

	if (!zend_hash_update_ptr(table, key, (void*) function)) {
		if (clazz) {
			uopz_exception(
				"failed to update zend function table while adding method %s::%s",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
		} else {
			uopz_exception(
				"failed to update zend function table while adding function %s",
				ZSTR_VAL(name));
		}
		zend_hash_del(functions, key);
		zend_string_release(key);
		destroy_zend_function(function);
		return 0;
	}

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
		if (flags == LONG_MAX) {
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

		current = clazz->ce_flags;
		clazz->ce_flags = flags;
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

	if (flags == LONG_MAX) {
		RETURN_LONG(function->common.fn_flags);
	}

	current = function->common.fn_flags;
	function->common.fn_flags = flags;
	RETURN_LONG(current);
} /* }}} */

static inline uopz_try_addref(zval *z) { /* {{{ */
	if (Z_REFCOUNTED_P(z)) {
		Z_ADDREF_P(z);
	}
} /* }}} */

void uopz_set_static(zend_class_entry *clazz, zend_string *function, zval *statics) { /* {{{ */
	zend_function *entry;
	zval *var = NULL;

	if (clazz) {
		if (uopz_find_function(&clazz->function_table, function, &entry) != SUCCESS) {
			return;
		}
	} else {
		if (uopz_find_function(CG(function_table), function, &entry) != SUCCESS) {
			return;
		}
	}

	if (entry->type != ZEND_USER_FUNCTION) {
		return;
	}

	if (!entry->op_array.static_variables) {
		return;
	}

	ZEND_HASH_FOREACH_VAL(entry->op_array.static_variables, var) {
		if (Z_REFCOUNTED_P(var)) {
			zval_ptr_dtor(var);
		}

		ZVAL_NULL(var);
	} ZEND_HASH_FOREACH_END();

	if (zend_hash_num_elements(Z_ARRVAL_P(statics))) {
		zend_hash_copy(
			entry->op_array.static_variables, 
			Z_ARRVAL_P(statics),
			(copy_ctor_func_t) uopz_try_addref);	
	}
} /* }}} */

void uopz_get_static(zend_class_entry *clazz, zend_string *function, zval *return_value) { /* {{{ */
	zend_function *entry;

	if (clazz) {
		if (uopz_find_function(&clazz->function_table, function, &entry) != SUCCESS) {
			return;
		}
	} else {
		if (uopz_find_function(CG(function_table), function, &entry) != SUCCESS) {
			return;
		}
	}

	if (entry->type != ZEND_USER_FUNCTION) {
		return;
	}

	if (!entry->op_array.static_variables) {
		return;
	}

	ZVAL_ARR(return_value, entry->op_array.static_variables);
	GC_REFCOUNT(entry->op_array.static_variables)++;
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
