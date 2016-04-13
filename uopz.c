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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_closures.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_inheritance.h"

#ifdef HAVE_SPL
#include "ext/spl/spl_exceptions.h"
#else
/* {{{ */
zend_class_entry *spl_ce_RuntimeException;
zend_class_entry *spl_ce_InvalidArgumentException; /* }}} */
#endif

#include "uopz.h"

#include "src/util.h"
#include "src/return.h"
#include "src/hook.h"
#include "src/constant.h"
#include "src/class.h"
#include "src/handlers.h"
#include "src/executors.h"
#include "src/function.h"

ZEND_DECLARE_MODULE_GLOBALS(uopz)

#define uopz_parse_parameters(spec, ...) zend_parse_parameters_ex\
	(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), spec, ##__VA_ARGS__)
#define uopz_refuse_parameters(message, ...) zend_throw_exception_ex\
	(spl_ce_InvalidArgumentException, 0, message, ##__VA_ARGS__)

/* {{{ */
static void php_uopz_init_globals(zend_uopz_globals *ng) {

} /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(uopz)
{
	ZEND_INIT_MODULE_GLOBALS(uopz, php_uopz_init_globals, NULL);

	REGISTER_LONG_CONSTANT("ZEND_ACC_PUBLIC", 				ZEND_ACC_PUBLIC, 				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_PRIVATE", 				ZEND_ACC_PRIVATE,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_PROTECTED", 			ZEND_ACC_PROTECTED,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_PPP_MASK", 			ZEND_ACC_PPP_MASK,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_STATIC", 				ZEND_ACC_STATIC,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_FINAL", 				ZEND_ACC_FINAL,					CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_ABSTRACT", 			ZEND_ACC_ABSTRACT,				CONST_CS|CONST_PERSISTENT);

	uopz_executors_init();
	uopz_handlers_init();

	return SUCCESS;
}
/* }}} */

/* {{{ */
static PHP_MSHUTDOWN_FUNCTION(uopz)
{
	uopz_executors_shutdown();
	uopz_handlers_shutdown();

	return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(uopz)
{
	zend_class_entry *ce = NULL;
	zend_string *spl;

#ifdef ZTS
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	spl = zend_string_init(ZEND_STRL("RuntimeException"), 0);
	spl_ce_RuntimeException =
			(ce = zend_lookup_class(spl)) ?
				ce : zend_exception_get_default();
	zend_string_release(spl);

	spl = zend_string_init(ZEND_STRL("InvalidArgumentException"), 0);
	spl_ce_InvalidArgumentException =
			(ce = zend_lookup_class(spl)) ?
				ce : zend_exception_get_default();
	zend_string_release(spl);

	uopz_request_init();

	return SUCCESS;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(uopz)
{
	uopz_request_shutdown();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(uopz)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "uopz support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto bool uopz_set_return(string class, string function, mixed value)
	   proto bool uopz_set_return(function, mixed value) */
static PHP_FUNCTION(uopz_set_return) 
{
	zend_string *function = NULL;
	zval *variable = NULL;
	zend_class_entry *clazz = NULL;
	zend_long execute = 0;

	if (uopz_parse_parameters("CSz|l", &clazz, &function, &variable, &execute) != SUCCESS &&
		uopz_parse_parameters("Sz|l", &function, &variable, &execute) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function, variable [, execute]) or (function, variable [, execute])");
		return;
	}

	if (execute && !instanceof_function(Z_OBJCE_P(variable), zend_ce_closure)) {
		uopz_refuse_parameters(
			"only closures are accepted as executable return values");
		return;
	}

	if (uopz_is_magic_method(clazz, function)) {
		uopz_refuse_parameters(
			"will not override magic methods, too magical");
		return;
	}

	RETURN_BOOL(uopz_set_return(clazz, function, variable, execute));
} /* }}} */

/* {{{ proto bool uopz_unset_return(string class, string function)
	   proto bool uopz_unset_return(string function) */
static PHP_FUNCTION(uopz_unset_return) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CS", &clazz, &function) != SUCCESS &&
		uopz_parse_parameters("S", &function) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}

	RETURN_BOOL(uopz_unset_return(clazz, function));
} /* }}} */

/* {{{ proto mixed uopz_get_return(string class, string function)
	   proto mixed uopz_get_return(string function) */
static PHP_FUNCTION(uopz_get_return) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CS", &clazz, &function) != SUCCESS &&
		uopz_parse_parameters("S", &function) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, function)");
		return;
	}

	uopz_get_return(clazz, function, return_value);
} /* }}} */

/* {{{ proto void uopz_set_mock(string class, mixed mock) */
static PHP_FUNCTION(uopz_set_mock) 
{
	zend_string *clazz = NULL;
	zval *mock = NULL;

	if (uopz_parse_parameters("Sz", &clazz, &mock) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, mock), classes not found ?");
		return;
	}

	if (!mock || (Z_TYPE_P(mock) != IS_STRING && Z_TYPE_P(mock) != IS_OBJECT)) {
		uopz_refuse_parameters(
			"unexpected parameter combination, mock is expected to be a string, or an object");
		return;
	}

	uopz_set_mock(clazz, mock);
} /* }}} */

/* {{{ proto void uopz_unset_mock(string mock) */
static PHP_FUNCTION(uopz_unset_mock) 
{
	zend_string *clazz = NULL;

	if (uopz_parse_parameters("S", &clazz) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (clazz), class not found ?");
		return;
	}

	uopz_unset_mock(clazz);
} /* }}} */

/* {{{ proto void uopz_get_mock(string mock) */
static PHP_FUNCTION(uopz_get_mock) 
{
	zend_string *clazz = NULL;

	if (uopz_parse_parameters("S", &clazz) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (clazz), class not found ?");
		return;
	}

	uopz_get_mock(clazz, return_value);
} /* }}} */

/* {{{ proto array uopz_get_static(string class, string method)
			 array uopz_get_static(string function) */
static PHP_FUNCTION(uopz_get_static) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CS", &clazz, &function) != SUCCESS &&
		uopz_parse_parameters("S", &function) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}

	uopz_get_static(clazz, function, return_value);
} /* }}} */

/* {{{ proto array uopz_set_static(string class, string method, array statics)
			 array uopz_set_static(string function, array statics) */
static PHP_FUNCTION(uopz_set_static) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;
	zval *statics = NULL;

	if (uopz_parse_parameters("CSz", &clazz, &function, &statics) != SUCCESS &&
		uopz_parse_parameters("Sz", &function, &statics) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function, statics) or (function, statics)");
		return;
	}

	uopz_set_static(clazz, function, statics);
} /* }}} */

/* {{{ proto bool uopz_set_hook(string class, string function, Closure hook)
			 bool uopz_set_hook(string function, Closure hook) */
static PHP_FUNCTION(uopz_set_hook) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;
	zval *hook = NULL;
	
	if (uopz_parse_parameters("CSO", &clazz, &function, &hook, zend_ce_closure) != SUCCESS &&
		uopz_parse_parameters("SO", &function, &hook, zend_ce_closure) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function, hook) or (function, hook)");
		return;
	}

	RETURN_BOOL(uopz_set_hook(clazz, function, hook));
} /* }}} */

/* {{{ proto bool uopz_unset_hook(string class, string function)
			 bool uopz_unset_hook(string function) */
static PHP_FUNCTION(uopz_unset_hook) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CS", &clazz, &function) != SUCCESS &&
		uopz_parse_parameters("S", &function) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}

	RETURN_BOOL(uopz_unset_hook(clazz, function));
} /* }}} */

/* {{{ proto Closure uopz_get_hook(string class, string function)
			 Closure uopz_get_hook(string function) */
static PHP_FUNCTION(uopz_get_hook) 
{
	zend_string *function = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CS", &clazz, &function) != SUCCESS &&
		uopz_parse_parameters("S", &function) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}	

	uopz_get_hook(clazz, function, return_value);
} /* }}} */

/* {{{ proto bool uopz_add_function(string class, string method, Closure function [, int flags = ZEND_ACC_PUBLIC])
			 bool uopz_add_function(string function, Closure function [, int flags = ZEND_ACC_PUBLIC]) */
static PHP_FUNCTION(uopz_add_function)
{
	zend_class_entry *clazz = NULL;
	zend_string *name = NULL;
	zval *closure = NULL;
	zend_long flags = ZEND_ACC_PUBLIC;

	if (uopz_parse_parameters("CSO|l", &clazz, &name, &closure, zend_ce_closure, &flags) != SUCCESS &&
		uopz_parse_parameters("SO|l", &name, &closure, zend_ce_closure, &flags) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, function, closure [, flags]) or (function, closure [, flags])");
		return;
	}

	RETURN_BOOL(uopz_add_function(clazz, name, closure, flags));
} /* }}} */

/* {{{ proto bool uopz_del_function(string class, string method)
			 bool uopz_del_function(string function) */
static PHP_FUNCTION(uopz_del_function)
{
	zend_class_entry *clazz = NULL;
	zend_string *name = NULL;

	if (uopz_parse_parameters("CS", &clazz, &name) != SUCCESS &&
		uopz_parse_parameters("S", &name) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}

	RETURN_BOOL(uopz_del_function(clazz, name));
} /* }}} */

/* {{{ proto bool uopz_redefine(string constant, mixed variable)
	   proto bool uopz_redefine(string class, string constant, mixed variable) */
static PHP_FUNCTION(uopz_redefine)
{
	zend_string *name = NULL;
	zval *variable = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CSz", &clazz, &name, &variable) != SUCCESS &&
		uopz_parse_parameters("Sz", &name, &variable) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, constant, variable) or (constant, variable)");
		return;
	}

	if (uopz_constant_redefine(clazz, name, variable)) {
		if (clazz) {
			while ((clazz = clazz->parent)) {
				uopz_constant_redefine(
					clazz, name, variable);
			}
		}
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
} /* }}} */

/* {{{ proto bool uopz_undefine(string constant)
	   proto bool uopz_undefine(string class, string constant) */
static PHP_FUNCTION(uopz_undefine)
{
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;

	if (uopz_parse_parameters("CS", &clazz, &name) != SUCCESS &&
		uopz_parse_parameters("S", &name) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, constant) or (constant)");
		return;
	}

	if (uopz_constant_undefine(clazz, name)) {
		if (clazz) {
			while ((clazz = clazz->parent)) {
				uopz_constant_undefine(clazz, name);
			}
		}
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
} /* }}} */

/* {{{ proto bool uopz_implement(string class, string interface) */
static PHP_FUNCTION(uopz_implement)
{
	zend_class_entry *clazz = NULL;
	zend_class_entry *interface = NULL;

	if (uopz_parse_parameters("CC", &clazz, &interface) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, interface)");
		return;
	}

	RETURN_BOOL(uopz_implement(clazz, interface));
} /* }}} */

/* {{{ proto bool uopz_extend(string class, string parent) */
static PHP_FUNCTION(uopz_extend)
{
	zend_class_entry *clazz = NULL;
	zend_class_entry *parent = NULL;

	if (uopz_parse_parameters("CC", &clazz, &parent) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, parent)");
		return;
	}

	RETURN_BOOL(uopz_extend(clazz, parent));
} /* }}} */

/* {{{ proto int uopz_flags(string function [, int flags])
       proto int uopz_flags(string class, string function [, int flags]) */
static PHP_FUNCTION(uopz_flags)
{
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;
	zend_long flags = LONG_MAX;

	if (uopz_parse_parameters("CS|l", &clazz, &name, &flags) != SUCCESS &&
		uopz_parse_parameters("S|l", &name, &flags) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected "
				"(class, function, flags) or (function, flags)");
		return;
	}	

	uopz_flags(clazz, name, flags, return_value);
} /* }}} */

/* {{{ proto void uopz_set_property(object instance, string property, mixed value) 
			 void uopz_set_property(string class, string property, mixed value) */
static PHP_FUNCTION(uopz_set_property) 
{
	zval *scope = NULL;
	zval *prop  = NULL;
	zval *value = NULL;

	if (uopz_parse_parameters("zzz", &scope, &prop, &value) != SUCCESS ||
		!scope || !prop || !value ||
		(Z_TYPE_P(scope) != IS_OBJECT && Z_TYPE_P(scope) != IS_STRING) ||
		Z_TYPE_P(prop) != IS_STRING) {
		uopz_refuse_parameters(
			"unexpected paramter combination, expected "
			"(class, property, value) or (object, property, value)");
		return;
	}

	if (Z_TYPE_P(scope) == IS_OBJECT) {
		uopz_set_property(scope, prop, value);
	} else {
		zend_class_entry *ce = zend_lookup_class(Z_STR_P(scope));
		
		if (!ce) {
			return;
		}

		uopz_set_static_property(ce, Z_STR_P(prop), value);
	}
} /* }}} */

/* {{{ proto mixed uopz_get_property(object instance, string property) 
	   proto mixed uopz_get_property(string class, string property) */
static PHP_FUNCTION(uopz_get_property) {
	zval *scope = NULL;
	zval *prop  = NULL;

	if (uopz_parse_parameters("zz", &scope, &prop) != SUCCESS ||
		!scope || !prop ||
		(Z_TYPE_P(scope) != IS_OBJECT && Z_TYPE_P(scope) != IS_STRING) ||
		Z_TYPE_P(prop) != IS_STRING) {
		uopz_refuse_parameters(
			"unexpected paramter combination, expected "
			"(class, property) or (object, property)");
		return;
	}

	if (Z_TYPE_P(scope) == IS_OBJECT) {
		uopz_get_property(scope, prop, return_value);
	} else {
		zend_class_entry *ce = zend_lookup_class(Z_STR_P(scope));
		
		if (!ce) {
			return;
		}

		uopz_get_static_property(ce, Z_STR_P(prop), return_value);
	}
} /* }}} */

/* {{{ uopz_functions[]
 */
#define UOPZ_FE(f) PHP_FE(f, NULL)
static const zend_function_entry uopz_functions[] = {
	UOPZ_FE(uopz_set_return)
	UOPZ_FE(uopz_get_return)
	UOPZ_FE(uopz_unset_return)
	UOPZ_FE(uopz_set_mock)
	UOPZ_FE(uopz_get_mock)
	UOPZ_FE(uopz_unset_mock)
	UOPZ_FE(uopz_get_static)
	UOPZ_FE(uopz_set_static)
	UOPZ_FE(uopz_set_hook)
	UOPZ_FE(uopz_get_hook)
	UOPZ_FE(uopz_unset_hook)
	UOPZ_FE(uopz_add_function)
	UOPZ_FE(uopz_del_function)
	UOPZ_FE(uopz_extend)
	UOPZ_FE(uopz_implement)
	UOPZ_FE(uopz_flags)
	UOPZ_FE(uopz_redefine)
	UOPZ_FE(uopz_undefine)
	UOPZ_FE(uopz_set_property)
	UOPZ_FE(uopz_get_property)
	{NULL, NULL, NULL}
};
#undef UOPZ_FE
/* }}} */

/* {{{ uopz_module_entry
 */
zend_module_entry uopz_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_UOPZ_EXTNAME,
	uopz_functions,
	PHP_MINIT(uopz),
	PHP_MSHUTDOWN(uopz),
	PHP_RINIT(uopz),
	PHP_RSHUTDOWN(uopz),
	PHP_MINFO(uopz),
	PHP_UOPZ_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_UOPZ
ZEND_GET_MODULE(uopz)
#ifdef ZTS
	ZEND_TSRMLS_CACHE_DEFINE();
#endif
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
