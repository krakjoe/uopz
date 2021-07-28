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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include "uopz.h"

#include "src/util.h"
#include "src/return.h"
#include "src/hook.h"
#include "src/constant.h"
#include "src/class.h"
#include "src/function.h"
#include "src/handlers.h"
#include "src/executors.h"

ZEND_DECLARE_MODULE_GLOBALS(uopz)

#define uopz_parse_parameters(spec, ...) zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), spec, ##__VA_ARGS__)
#define uopz_refuse_parameters(message, ...) zend_throw_exception_ex\
	(spl_ce_InvalidArgumentException, 0, message, ##__VA_ARGS__)

#define uopz_disabled_guard() do { \
	if (UOPZ(disable)) { \
		zend_throw_exception_ex(spl_ce_RuntimeException, 0, "uopz is disabled by configuration (uopz.disable)"); \
		return; \
	} \
} while(0)

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("uopz.disable", "0", PHP_INI_SYSTEM, OnUpdateBool, disable, zend_uopz_globals, uopz_globals)
	STD_PHP_INI_ENTRY("uopz.exit",    "0", PHP_INI_SYSTEM, OnUpdateBool, exit,    zend_uopz_globals, uopz_globals)
PHP_INI_END()

/* {{{ */
static void php_uopz_init_globals(zend_uopz_globals *ng) {
	memset(ng, 0, sizeof(zend_uopz_globals));
} /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(uopz)
{
	ZEND_INIT_MODULE_GLOBALS(uopz, php_uopz_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	if (UOPZ(disable)) {
		return SUCCESS;
	}

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
	if (UOPZ(disable)) {
		return SUCCESS;
	}

	uopz_handlers_shutdown();
	uopz_executors_shutdown();

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

	if (UOPZ(disable)) {
		return SUCCESS;
	}

	if (INI_INT("opcache.optimization_level")) {

		zend_string *optimizer = zend_string_init(
			ZEND_STRL("opcache.optimization_level"), 1);
		zend_long level = INI_INT("opcache.optimization_level");
		zend_string *value;

		/* must disable block pass 1 constant substitution */
		level &= ~(1<<0);
		
		/* disable CFG optimization (exit optimized away here) */
		level &= ~(1<<4);

		/* disable DCE (want code after exit) */
		level &= ~(1<<13);

		value = strpprintf(0, "0x%08X", (unsigned int) level);

		zend_alter_ini_entry(optimizer, value,
			ZEND_INI_SYSTEM, ZEND_INI_STAGE_ACTIVATE);

		zend_string_release(optimizer);
		zend_string_release(value);
	}

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
	if (UOPZ(disable)) {
		return SUCCESS;
	}

	uopz_request_shutdown();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(uopz)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "uopz support", UOPZ(disable) ? "disabled" : "enabled");
	php_info_print_table_row(2, "Version", PHP_UOPZ_VERSION);
	php_info_print_table_end();

 	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ proto bool uopz_set_return(string class, string function, mixed variable [, bool execute ])
	   proto bool uopz_set_return(function, mixed variable [, bool execute ]) */
static PHP_FUNCTION(uopz_set_return) 
{
	zend_string *function = NULL;
	zval *variable = NULL;
	zend_class_entry *clazz = NULL;
	zend_bool execute = 0;

	uopz_disabled_guard();

	if (uopz_parse_parameters("CSz|b", &clazz, &function, &variable, &execute) != SUCCESS &&
		uopz_parse_parameters("Sz|b", &function, &variable, &execute) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function, variable [, execute]) or (function, variable [, execute])");
		return;
	}

	if (execute && (Z_TYPE_P(variable) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(variable), zend_ce_closure))) {
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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();
	
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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

	if (uopz_parse_parameters("CS", &clazz, &function) != SUCCESS &&
		uopz_parse_parameters("S", &function) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}	

	uopz_get_hook(clazz, function, return_value);
} /* }}} */

/* {{{ proto bool uopz_add_function(string class, string method, Closure function [, int flags = ZEND_ACC_PUBLIC [, bool all = false]])
			 bool uopz_add_function(string function, Closure function [, int flags = ZEND_ACC_PUBLIC]) */
static PHP_FUNCTION(uopz_add_function)
{
	zend_class_entry *clazz = NULL;
	zend_string *name = NULL;
	zval *closure = NULL;
	zend_long flags = ZEND_ACC_PUBLIC;
	zend_bool all = 1;

	uopz_disabled_guard();

	if (uopz_parse_parameters("CSO|lb", &clazz, &name, &closure, zend_ce_closure, &flags, &all) != SUCCESS &&
		uopz_parse_parameters("SO|l", &name, &closure, zend_ce_closure, &flags) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, function, closure [, flags]) or (function, closure [, flags])");
		return;
	}

	RETURN_BOOL(uopz_add_function(clazz, name, closure, flags, all));
} /* }}} */

/* {{{ proto bool uopz_del_function(string class, string method [, bool all = false])
			 bool uopz_del_function(string function) */
static PHP_FUNCTION(uopz_del_function)
{
	zend_class_entry *clazz = NULL;
	zend_string *name = NULL;
	zend_bool all = 1;

	uopz_disabled_guard();

	if (uopz_parse_parameters("CS|b", &clazz, &name, &all) != SUCCESS &&
		uopz_parse_parameters("S", &name) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, function) or (function)");
		return;
	}

	RETURN_BOOL(uopz_del_function(clazz, name, all));
} /* }}} */

/* {{{ proto bool uopz_redefine(string constant, mixed variable)
	   proto bool uopz_redefine(string class, string constant, mixed variable) */
static PHP_FUNCTION(uopz_redefine)
{
	zend_string *name = NULL;
	zval *variable = NULL;
	zend_class_entry *clazz = NULL;

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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
	zend_long flags = ZEND_LONG_MAX;

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

	uopz_disabled_guard();

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

/* {{{ proto mixed uopz_get_exit_status(void) */
static PHP_FUNCTION(uopz_get_exit_status) {

	uopz_disabled_guard();

	if (Z_TYPE(UOPZ(estatus)) != IS_UNDEF) {
		ZVAL_COPY(return_value, &UOPZ(estatus));
	}
} /* }}} */

/* {{{ proto mixed uopz_allow_exit(bool allow) */
static PHP_FUNCTION(uopz_allow_exit) {
	zend_bool allow = 0;

	uopz_disabled_guard();
	
	if (uopz_parse_parameters("b", &allow) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (allow)");
		return;
	}

	UOPZ(exit) = allow;
} /* }}} */

/* {{{ uopz_functions[]
 */
ZEND_BEGIN_ARG_INFO(uopz_ignore_arginfo, 1)
	ZEND_ARG_VARIADIC_INFO(0, arguments)
ZEND_END_ARG_INFO()
# define UOPZ_FE(f) PHP_FE(f, uopz_ignore_arginfo)

ZEND_BEGIN_ARG_INFO(uopz_no_args_arginfo, 0)
ZEND_END_ARG_INFO()
# define UOPZ_FE_NOARGS(f) PHP_FE(f, uopz_no_args_arginfo)

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
	UOPZ_FE_NOARGS(uopz_get_exit_status)
	UOPZ_FE(uopz_allow_exit)

	UOPZ_FE(uopz_call_user_func)
	UOPZ_FE(uopz_call_user_func_array)
	ZEND_FE_END
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
