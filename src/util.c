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

#ifndef UOPZ_UTIL
#define UOPZ_UTIL

#include "php.h"
#include "uopz.h"

#include "util.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

zend_internal_function *zend_call_user_func;
zend_internal_function *zend_call_user_func_array;

static inline void uopz_table_dtor(zval *zv) { /* {{{ */
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
} /* }}} */

/* {{{ */
typedef struct _uopz_magic_t {
	const char *name;
	size_t      length;
	int         id;
} uopz_magic_t;

#define UOPZ_MAGIC(name, id) {name, sizeof(name)-1, id}
#define UOPZ_MAGIC_END	     {NULL, 0, 0L}

static const uopz_magic_t umagic[] = {
	UOPZ_MAGIC(ZEND_CONSTRUCTOR_FUNC_NAME, 0),
	UOPZ_MAGIC(ZEND_DESTRUCTOR_FUNC_NAME, 1),
	UOPZ_MAGIC(ZEND_CLONE_FUNC_NAME, 2),
	UOPZ_MAGIC(ZEND_GET_FUNC_NAME, 3),
	UOPZ_MAGIC(ZEND_SET_FUNC_NAME, 4),
	UOPZ_MAGIC(ZEND_UNSET_FUNC_NAME, 5),
	UOPZ_MAGIC(ZEND_ISSET_FUNC_NAME, 6),
	UOPZ_MAGIC(ZEND_CALL_FUNC_NAME, 7),
	UOPZ_MAGIC(ZEND_CALLSTATIC_FUNC_NAME, 8),
	UOPZ_MAGIC(ZEND_TOSTRING_FUNC_NAME, 9),
	UOPZ_MAGIC("serialize", 10),
	UOPZ_MAGIC("unserialize", 11),
	UOPZ_MAGIC(ZEND_DEBUGINFO_FUNC_NAME, 12),
	UOPZ_MAGIC_END
};

void uopz_handle_magic(zend_class_entry *clazz, zend_string *name, zend_function *function) { /* {{{ */
	uopz_magic_t *magic;

	for (magic = (uopz_magic_t*) umagic; magic->name; magic++) {
		if (ZSTR_LEN(name) == magic->length &&
				strncasecmp(ZSTR_VAL(name), magic->name, magic->length) == SUCCESS) {

			switch (magic->id) {
				case 0: clazz->constructor = function; break;
				case 1: clazz->destructor = function; break;
				case 2: clazz->clone = function; break;
				case 3: clazz->__get = function; break;
				case 4: clazz->__set = function; break;
				case 5: clazz->__unset = function; break;
				case 6: clazz->__isset = function; break;
				case 7: clazz->__call = function; break;
				case 8: clazz->__callstatic = function; break;
				case 9: clazz->__tostring = function; break;
				case 10: clazz->serialize_func = function; break;
				case 11: clazz->unserialize_func = function; break;
				case 12: clazz->__debugInfo = function; break;
			}
			return;
		}
	}
} /* }}} */

int uopz_find_method(zend_class_entry *ce, zend_string *name, zend_function **function) { /* {{{ */
	return uopz_find_function(&ce->function_table, name, function);
} /* }}} */

int uopz_find_function(HashTable *table, zend_string *name, zend_function **function) { /* {{{ */
	zend_string *key = zend_string_tolower(name);
	zend_function *ptr = zend_hash_find_ptr(table, key);

	zend_string_release(key);

	if (!ptr) {
		return FAILURE;
	}

	if (function) {
		*function = ptr;
	}

	return SUCCESS;
} /* }}} */

zend_bool uopz_is_magic_method(zend_class_entry *clazz, zend_string *function) /* {{{ */
{ 
	if (!clazz) {
		return 0;
	}

	if (zend_string_equals_literal_ci(function, "__construct") ||
		zend_string_equals_literal_ci(function, "__destruct") ||
		zend_string_equals_literal_ci(function, "__clone") ||
		zend_string_equals_literal_ci(function, "__get") ||
		zend_string_equals_literal_ci(function, "__set") ||
		zend_string_equals_literal_ci(function, "__unset") ||
		zend_string_equals_literal_ci(function, "__isset") ||
		zend_string_equals_literal_ci(function, "__call") ||
		zend_string_equals_literal_ci(function, "__callstatic") ||
		zend_string_equals_literal_ci(function, "__tostring") ||
		zend_string_equals_literal_ci(function, "__debuginfo") ||
		zend_string_equals_literal_ci(function, "__serialize") ||
		zend_string_equals_literal_ci(function, "__unserialize") ||
		zend_string_equals_literal_ci(function, "__sleep") ||
		zend_string_equals_literal_ci(function, "__wakeup")) {
		return 1;
	}

	return 0;
} /* }}} */

static inline int uopz_closure_equals(zval *closure, zend_function *function) { /* {{{ */
	const zend_function *cmp = zend_get_closure_method_def(closure);

	if (cmp == function) {
		return 1;
	}

	if (cmp->type == function->type && 
		cmp->op_array.opcodes == function->op_array.opcodes) {
		return 1;
	}

	return 0;
} /* }}} */

int uopz_clean_function(zval *zv) { /* {{{ */
	zend_function *fp = Z_PTR_P(zv);

	if (fp->common.fn_flags & ZEND_ACC_CLOSURE) {
		HashTable *table = fp->common.scope ?
			&fp->common.scope->function_table : CG(function_table);

		HashTable *functions = 
			zend_hash_index_find_ptr(&UOPZ(functions), (zend_long) table);

		if (functions) {
			zval *closure = NULL;

			ZEND_HASH_FOREACH_VAL(functions, closure) {
				if (uopz_closure_equals(closure, fp)) {
					return ZEND_HASH_APPLY_REMOVE;
				}
			} ZEND_HASH_FOREACH_END();
		}
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

int uopz_clean_class(zval *zv) { /* {{{ */
	zend_class_entry *ce = Z_PTR_P(zv);

	zend_hash_apply(
		&ce->function_table, uopz_clean_function);
	
	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

static void uopz_callers_init(void) { /* {{{ */
	zend_internal_function *internal = zend_hash_str_find_ptr(
		CG(function_table), "uopz_set_return", sizeof("uopz_set_return")-1);

	do {
		zend_call_user_func = zend_hash_str_find_ptr(
			CG(function_table), "call_user_func", sizeof("call_user_func")-1);
		{
			zend_internal_function stack;
			zend_internal_function *uopz = zend_hash_str_find_ptr(
				CG(function_table), "uopz_call_user_func", sizeof("uopz_call_user_func")-1);

			if (zend_call_user_func->module == internal->module) {
				break;
			}

			stack = *zend_call_user_func;
		
			memcpy(zend_call_user_func, uopz, sizeof(zend_internal_function));
			memcpy(uopz, &stack, sizeof(zend_internal_function));
		}
	} while (0);

	do {
		zend_call_user_func_array = zend_hash_str_find_ptr(
			CG(function_table), "call_user_func_array", sizeof("call_user_func_array")-1);
		{
			zend_internal_function stack;
			zend_internal_function *uopz = zend_hash_str_find_ptr(
				CG(function_table), "uopz_call_user_func_array", sizeof("uopz_call_user_func_array")-1);

			if (zend_call_user_func->module == internal->module) {
				break;
			}

			stack = *zend_call_user_func_array;
		
			memcpy(zend_call_user_func_array, uopz, sizeof(zend_internal_function));
			memcpy(uopz, &stack, sizeof(zend_internal_function));
		}
	} while (0);
} /* }}} */

static void uopz_callers_shutdown(void) { /* {{{ */
	zend_internal_function *internal = zend_hash_str_find_ptr(
		CG(function_table), "uopz_set_return", sizeof("uopz_set_return")-1);

	do {
		zend_call_user_func = zend_hash_str_find_ptr(
			CG(function_table), "call_user_func", sizeof("call_user_func")-1);
		{
			zend_internal_function stack;
			zend_internal_function *uopz = zend_hash_str_find_ptr(
				CG(function_table), "uopz_call_user_func", sizeof("uopz_call_user_func")-1);

			if (zend_call_user_func->module != internal->module) {
				break;
			}

			stack = *zend_call_user_func;
		
			memcpy(zend_call_user_func, uopz, sizeof(zend_internal_function));
			memcpy(uopz, &stack, sizeof(zend_internal_function));
		}
	} while (0);

	do {
		zend_call_user_func_array = zend_hash_str_find_ptr(
			CG(function_table), "call_user_func_array", sizeof("call_user_func_array")-1);
		{
			zend_internal_function stack;
			zend_internal_function *uopz = zend_hash_str_find_ptr(
				CG(function_table), "uopz_call_user_func_array", sizeof("uopz_call_user_func_array")-1);

			if (zend_call_user_func->module != internal->module) {
				break;
			}

			stack = *zend_call_user_func_array;
		
			memcpy(zend_call_user_func_array, uopz, sizeof(zend_internal_function));
			memcpy(uopz, &stack, sizeof(zend_internal_function));
		}
	} while (0);
} /* }}} */

void uopz_request_init(void) { /* {{{ */
	UOPZ(copts) = CG(compiler_options);

	CG(compiler_options) |= ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION | 
#ifdef ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION
							ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION |
#endif
							ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS | 
							ZEND_COMPILE_IGNORE_USER_FUNCTIONS | 
							ZEND_COMPILE_GUARDS;

	zend_hash_init(&UOPZ(functions), 8, NULL, uopz_table_dtor, 0);
	zend_hash_init(&UOPZ(returns), 8, NULL, uopz_table_dtor, 0);
	zend_hash_init(&UOPZ(mocks), 8, NULL, uopz_zval_dtor, 0);
	zend_hash_init(&UOPZ(hooks), 8, NULL, uopz_table_dtor, 0);

	{
		char *report = getenv("UOPZ_REPORT_MEMLEAKS");

		PG(report_memleaks) = 
			(report && report[0] == '1');
	}

	uopz_callers_init();
} /* }}} */

void uopz_request_shutdown(void) { /* {{{ */
	CG(compiler_options) = UOPZ(copts);

	zend_hash_apply(CG(class_table), uopz_clean_class);
	zend_hash_apply(CG(function_table), uopz_clean_function);

	zend_hash_destroy(&UOPZ(functions));
	zend_hash_destroy(&UOPZ(mocks));
	zend_hash_destroy(&UOPZ(returns));
	zend_hash_destroy(&UOPZ(hooks));

	uopz_callers_shutdown();
} /* }}} */

#endif	/* UOPZ_UTIL */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
