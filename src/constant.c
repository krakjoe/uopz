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

#ifndef UOPZ_CONSTANT
#define UOPZ_CONSTANT

#include "php.h"
#include "uopz.h"

#include "util.h"
#include "constant.h"

/* {{{ */
zend_bool uopz_constant_redefine(zend_class_entry *clazz, zend_string *name, zval *variable) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);

	if (!(zconstant = zend_hash_find_ptr(table, name))) {
		if (!clazz) {
			zend_constant create;

			ZVAL_COPY(&create.value, variable);
#if PHP_VERSION_ID < 70300
			create.flags = CONST_CS;
			create.module_number = PHP_USER_CONSTANT;
#else
			ZEND_CONSTANT_SET_FLAGS(&create, CONST_CS, PHP_USER_CONSTANT);
#endif
			create.name = zend_string_copy(name);

			zend_register_constant(&create);
		} else {
			zend_declare_class_constant(clazz, 
				ZSTR_VAL(name), ZSTR_LEN(name), variable);
			Z_TRY_ADDREF_P(variable);
		}

		return 1;
	}

	if (!clazz) {
#if PHP_VERSION_ID < 70300
		if (zconstant->module_number == PHP_USER_CONSTANT) {
#else
		if (ZEND_CONSTANT_MODULE_NUMBER(zconstant) == PHP_USER_CONSTANT) {
#endif
			zval_dtor(&zconstant->value);
			ZVAL_COPY(&zconstant->value, variable);
		} else {
			uopz_exception(
				"failed to redefine the internal %s, not allowed", ZSTR_VAL(name));
			return 0;
		}

	} else {
		zend_hash_del(table, name);

		zend_declare_class_constant(clazz, 
			ZSTR_VAL(name), ZSTR_LEN(name), variable);
		Z_TRY_ADDREF_P(variable);
	}

	return 1;
} /* }}} */

/* {{{ */
zend_bool uopz_constant_undefine(zend_class_entry *clazz, zend_string *name) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);

	if (!(zconstant = zend_hash_find_ptr(table, name))) {
		return 0;
	}

	if (!clazz) {
#if PHP_VERSION_ID < 70300
		if (zconstant->module_number != PHP_USER_CONSTANT) {
#else
		if (ZEND_CONSTANT_MODULE_NUMBER(zconstant) != PHP_USER_CONSTANT) {
#endif
			uopz_exception(
				"failed to undefine the internal constant %s, not allowed", ZSTR_VAL(name));
			return 0;
		}

		zend_hash_del(table, name);

		return 1;
	}

	zend_hash_del(table, name);

	return 1;
} /* }}} */

#endif	/* UOPZ_CONSTANT */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
