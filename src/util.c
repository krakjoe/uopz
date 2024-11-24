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

#ifndef UOPZ_UTIL
#define UOPZ_UTIL

#include "php.h"
#include "uopz.h"

#include "class.h"
#include "function.h"
#include "hook.h"
#include "return.h"
#include "util.h"

#include <Zend/zend_closures.h>

ZEND_EXTERN_MODULE_GLOBALS(uopz);

static zend_internal_function *zend_call_user_func_ptr;
static zend_internal_function *zend_call_user_func_array_ptr;
static zend_internal_function *uopz_call_user_func_ptr;
static zend_internal_function *uopz_call_user_func_array_ptr;
#ifndef ZEND_EXIT
static zend_internal_function *zend_exit_func_ptr;
static zend_internal_function *uopz_exit_func_ptr;
#endif

static inline void uopz_table_dtor(zval *zv) { /* {{{ */
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
} /* }}} */

/* {{{ */
typedef struct _uopz_magic_t {
	const char *name;
	size_t	  	length;
	int		 	id;
} uopz_magic_t;

#define UOPZ_MAGIC(name, id) {name, sizeof(name)-1, id}
#define UOPZ_MAGIC_END		 {NULL, 0, 0L}

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
	UOPZ_MAGIC("__serialize", 10),
	UOPZ_MAGIC("__unserialize", 11),
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
				case 10: clazz->__serialize = function; break;
				case 11: clazz->__unserialize = function; break;
				case 12: clazz->__debugInfo = function; break;
			}
			return;
		}
	}
} /* }}} */

zend_function *uopz_find_method(zend_class_entry *ce, zend_string *name) { /* {{{ */
	return uopz_find_function(&ce->function_table, name);
} /* }}} */

zend_function *uopz_find_function(HashTable *table, zend_string *name) { /* {{{ */
	zend_string *key = zend_string_tolower(name);
	zend_function *ptr = zend_hash_find_ptr(table, key);
	zend_string_release(key);
	return ptr;
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

int uopz_clean_function(zval *zv) { /* {{{ */
	zend_function *fp = Z_PTR_P(zv);

	if (fp->type == ZEND_USER_FUNCTION && fp->common.fn_flags & ZEND_ACC_UOPZ) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

int uopz_clean_class(zval *zv) { /* {{{ */
	zend_class_entry *ce = Z_PTR_P(zv);

	if (ce->ce_flags & ZEND_ACC_IMMUTABLE) {
		return ZEND_HASH_APPLY_KEEP;
	}
	
	zend_hash_apply(
		&ce->function_table, uopz_clean_function);
	
	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

static inline void uopz_caller_switch(zif_handler *old, zif_handler *new) {
	zif_handler *current = old;

	*old = *new;

	*new = *current;
}

static void uopz_callers_init(void) { /* {{{ */
	 uopz_call_user_func_ptr = zend_hash_str_find_ptr(
			CG(function_table), "uopz_call_user_func", sizeof("uopz_call_user_func")-1);
	 uopz_call_user_func_array_ptr = zend_hash_str_find_ptr(
			CG(function_table), "uopz_call_user_func_array", sizeof("uopz_call_user_func_array")-1);
	zend_call_user_func_ptr = zend_hash_str_find_ptr(
			CG(function_table), "call_user_func", sizeof("call_user_func")-1);
	zend_call_user_func_array_ptr = zend_hash_str_find_ptr(
			CG(function_table), "call_user_func_array", sizeof("call_user_func_array")-1);

	uopz_caller_switch(&zend_call_user_func_ptr->handler, &uopz_call_user_func_ptr->handler);
	uopz_caller_switch(&zend_call_user_func_array_ptr->handler, &uopz_call_user_func_array_ptr->handler);

#ifndef ZEND_EXIT
	uopz_exit_func_ptr = zend_hash_str_find_ptr(
			CG(function_table), "uopz_exit", sizeof("uopz_exit")-1);
	zend_exit_func_ptr = zend_hash_str_find_ptr(
			CG(function_table), "exit", sizeof("exit")-1);

	uopz_caller_switch(&zend_exit_func_ptr->handler, &uopz_exit_func_ptr->handler);
	zend_exit_func_ptr->fn_flags &= ~ZEND_ACC_HAS_RETURN_TYPE;
#endif
} /* }}} */

static void uopz_callers_shutdown(void) { /* {{{ */
	uopz_caller_switch(&uopz_call_user_func_ptr->handler, &zend_call_user_func_ptr->handler);
	uopz_caller_switch(&uopz_call_user_func_array_ptr->handler, &zend_call_user_func_array_ptr->handler);

#ifndef ZEND_EXIT
	uopz_caller_switch(&uopz_exit_func_ptr->handler, &zend_exit_func_ptr->handler);
	zend_exit_func_ptr->fn_flags |= ZEND_ACC_HAS_RETURN_TYPE;
#endif
} /* }}} */

#define UOPZ_CALL_HOOKS(variadic) do { \
	if (!fcc.function_handler) { \
		break; \
	} \
	\
	{ \
		uopz_hook_t *uhook = uopz_find_hook(fcc.function_handler); \
		\
		if (uhook && !uhook->busy) { \
			uopz_execute_hook(uhook, execute_data, 1, variadic); \
		} \
	} \
	\
	do { \
		uopz_return_t *ureturn = uopz_find_return(fcc.function_handler); \
		\
		if (ureturn) { \
			if (UOPZ_RETURN_IS_EXECUTABLE(ureturn)) { \
				if (UOPZ_RETURN_IS_BUSY(ureturn)) { \
					break; \
				} \
				\
				uopz_execute_return(ureturn, execute_data, return_value); \
				return; \
			} \
			\
			ZVAL_COPY(return_value, &ureturn->value); \
			return; \
		} \
	} while (0); \
} while (0)
/* {{{ proto mixed uopz_call_user_func(callable function, ... args) */
PHP_FUNCTION(uopz_call_user_func) {
	zval retval;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_FUNC(fci, fcc)
		Z_PARAM_VARIADIC_WITH_NAMED(fci.params, fci.param_count, fci.named_params)
	ZEND_PARSE_PARAMETERS_END();

	fci.retval = &retval;

	UOPZ_CALL_HOOKS(0);

	if (zend_call_function(&fci, &fcc) == SUCCESS && Z_TYPE(retval) != IS_UNDEF) {
		if (Z_ISREF(retval)) {
			zend_unwrap_reference(&retval);
		}
		ZVAL_COPY_VALUE(return_value, &retval);
	}
} /* }}} */

/* {{{ proto mixed uopz_call_user_func_array(callable function, array args) */
PHP_FUNCTION(uopz_call_user_func_array) {
	HashTable *params;
	zval retval;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_FUNC(fci, fcc)
		Z_PARAM_ARRAY_HT(params)
	ZEND_PARSE_PARAMETERS_END();

	fci.named_params = params;
	fci.retval = &retval;

	UOPZ_CALL_HOOKS(1);

	if (zend_call_function(&fci, &fcc) == SUCCESS && Z_TYPE(retval) != IS_UNDEF) {
		if (Z_ISREF(retval)) {
			zend_unwrap_reference(&retval);
		}
		ZVAL_COPY_VALUE(return_value, &retval);
	}
} /* }}} */

#ifndef ZEND_EXIT
PHP_FUNCTION(uopz_exit) {
	zend_string *str = NULL;
	zend_long status = 0;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR_OR_LONG(str, status)
	ZEND_PARSE_PARAMETERS_END();

	if (!UOPZ(exit)) {
		if (str != NULL) {
			ZVAL_STR(&UOPZ(estatus), str);
		} else {
			ZVAL_LONG(&UOPZ(estatus), status);
		}
		EG(exit_status) = status;
	} else {
		if (str) {
			size_t len = ZSTR_LEN(str);
			if (len != 0) {
				/* An exception might be emitted by an output handler */
				zend_write(ZSTR_VAL(str), len);
				if (EG(exception)) {
					RETURN_THROWS();
				}
			}
		} else {
			EG(exit_status) = status;
		}

		ZEND_ASSERT(!EG(exception));
		zend_throw_unwind_exit();
	}
}
#endif

void uopz_request_init(void) { /* {{{ */
	UOPZ(copts) = CG(compiler_options);

	CG(compiler_options) |=
				ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION |
				ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION |
				ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS | 
				ZEND_COMPILE_IGNORE_USER_FUNCTIONS | 
				ZEND_COMPILE_GUARDS;

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

	zend_hash_apply(CG(class_table),	uopz_clean_class);
	zend_hash_apply(CG(function_table), uopz_clean_function);

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
