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

#ifndef UOPZ_FUNCTION
#define UOPZ_FUNCTION

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "function.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

#define ZEND_ACC_UOPZ (1<<30)

static zend_function* uopz_copy_function(zend_class_entry *scope, zend_string *name, zend_object *closure, zend_long flags) { /* {{{ */
	zend_function  *copy = (zend_function*) zend_arena_alloc(&CG(arena), sizeof(zend_op_array));

	memcpy(copy, zend_get_closure_method_def(closure), sizeof(zend_op_array));

	copy->op_array.fn_flags &= ~ZEND_ACC_CLOSURE|ZEND_ACC_IMMUTABLE|ZEND_ACC_ARENA_ALLOCATED;

	copy->op_array.function_name = zend_string_copy(name);

	copy->op_array.scope = scope;

	if (flags & ZEND_ACC_PPP_MASK) {
		copy->op_array.fn_flags &= ~ZEND_ACC_PPP_MASK;
		copy->op_array.fn_flags |= flags & ZEND_ACC_PPP_MASK;
	} else {
		copy->op_array.fn_flags |= ZEND_ACC_PUBLIC;
	}

	if (flags & ZEND_ACC_STATIC) {
		copy->op_array.fn_flags |= ZEND_ACC_STATIC;
	}

	if (copy->op_array.static_variables) {
		copy->op_array.static_variables = zend_array_dup(copy->op_array.static_variables);

		ZEND_MAP_PTR_INIT(
			copy->op_array.static_variables_ptr, &copy->op_array.static_variables);
	} else {
		ZEND_MAP_PTR_INIT(copy->op_array.static_variables_ptr, NULL);
	}

	if (copy->op_array.refcount) {
		(*copy->op_array.refcount)++;
	}
	copy->op_array.fn_flags |= ZEND_ACC_UOPZ;

	return copy;
} /* }}} */

zend_bool uopz_add_function(zend_class_entry *clazz, zend_string *name, zval *closure, zend_long flags, zend_bool all) { /* {{{ */
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_string *key;
	zend_function *function = NULL;

	if (clazz && clazz->ce_flags & ZEND_ACC_IMMUTABLE) {
		uopz_exception(
			"cannot add method %s::%s to immutable class, use uopz_set_return instead",
			ZSTR_VAL(clazz->name),
			ZSTR_VAL(name));
		return 0;
	}

	key = zend_string_tolower(name);
	key = zend_new_interned_string(key);

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

	function = uopz_copy_function(clazz, name, Z_OBJ_P(closure), flags);
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
	zend_string *key = zend_string_tolower(name);
	zend_function *function = zend_hash_find_ptr(table, key);
	
	if (!function) {
		if (clazz) {
			uopz_exception(
				"cannot delete method %s::%s, it does not exist",
				ZSTR_VAL(clazz->name),
				ZSTR_VAL(name));
		} else {
			uopz_exception(
				"cannot delete function %s, it does not exist",
				ZSTR_VAL(name));
		}
		zend_string_release(key);
		return 0;
	}
	
	if (!(function->common.fn_flags & ZEND_ACC_UOPZ)) {
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
	
	if (clazz && all) {
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

	zend_hash_del(table, key);
	zend_string_release(key);

	return 1;
} /* }}} */

/* {{{ */
void uopz_flags(zend_class_entry *clazz, zend_string *name, zend_long flags, zval *return_value) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_long current = 0;

	if (clazz && (!name || !ZSTR_LEN(name))) {
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

		if (clazz->ce_flags & ZEND_ACC_IMMUTABLE) {
			uopz_exception(
				"attempt to set flags of immutable class entry %s, not allowed",
				ZSTR_VAL(clazz->name));
			return;
		}

		current = clazz->ce_flags;
		clazz->ce_flags = flags;
		if (current & ZEND_ACC_LINKED) {
			clazz->ce_flags |= ZEND_ACC_LINKED;
		}
		RETURN_LONG(current);
	}

	zend_function *function = uopz_find_function(table, name);
	if (!function) {
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
		if (function->common.fn_flags & ZEND_ACC_IMMUTABLE) {
			uopz_exception(
				"attempt to set flags of immutable function entry %s, not allowed",
				ZSTR_VAL(name));
			return;
		}

		/* Only allow changing a whitelist of flags, don't allow modifying internal flags. */
		uint32_t allowed_flags = ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC | ZEND_ACC_FINAL;
		function->common.fn_flags =
			(function->common.fn_flags & ~allowed_flags) | (flags & allowed_flags);
	}
	RETURN_LONG(current);
} /* }}} */

zend_bool uopz_set_static(zend_class_entry *clazz, zend_string *function, zval *statics) { /* {{{ */
	zend_function *entry;
	zend_string *k = NULL;
	zval *v = NULL;
	
	if (clazz) {
		entry = uopz_find_function(&clazz->function_table, function);
		if (!entry) {
			uopz_exception(
				"failed to set statics in method %s::%s, it does not exist",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
			return 0;
		}
	} else {
		entry = uopz_find_function(CG(function_table), function);
		if (!entry) {
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
	
	HashTable *variables = ZEND_MAP_PTR_GET(entry->op_array.static_variables_ptr);

	if (!variables) {
		ZEND_MAP_PTR_INIT(
			entry->op_array.static_variables_ptr, 
			&entry->op_array.static_variables);
		
		variables = ZEND_MAP_PTR_GET(entry->op_array.static_variables_ptr);
	}

	ZEND_HASH_FOREACH_STR_KEY_VAL(variables, k, v) {
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
		entry = uopz_find_function(&clazz->function_table, function);
		if (!entry) {
			uopz_exception(
				"failed to get statics from method %s::%s, it does not exist",
				ZSTR_VAL(clazz->name), ZSTR_VAL(function));
			return 0;
		}
	} else {
		entry = uopz_find_function(CG(function_table), function);
		if (!entry) {
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

	HashTable *variables = ZEND_MAP_PTR_GET(entry->op_array.static_variables_ptr);

	if (!variables) {
		variables = zend_array_dup(entry->op_array.static_variables);
		ZEND_MAP_PTR_SET(entry->op_array.static_variables_ptr, variables);
	}

	zval *val;
	ZEND_HASH_FOREACH_VAL(variables, val) {
		if (zval_update_constant_ex(val, entry->common.scope) != SUCCESS) {
			return false;
		}
	} ZEND_HASH_FOREACH_END();

	ZVAL_ARR(return_value, zend_array_dup(variables));
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
