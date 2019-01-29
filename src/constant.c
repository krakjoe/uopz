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
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);
	zend_string   *key = zend_string_copy(name);
	zend_constant *zconstant = zend_hash_find_ptr(table, key);

	if (!zconstant && !clazz) {
		char *ns = zend_memrchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name));
		size_t nss;

		if (ns) {
			zend_string *heap = zend_string_tolower(key);	
		
			ns++;
			nss =  (ZSTR_VAL(name) + ZSTR_LEN(name)) - ns;
			
			memcpy(&ZSTR_VAL(heap)[ZSTR_LEN(heap) - nss], ns, nss);
			
			zconstant = zend_hash_find_ptr(table, heap);

			zend_string_release(key);

			key = heap;
		}
	}

	if (!zconstant) {
		if (!clazz) {
			zend_constant create;

			ZVAL_COPY(&create.value, variable);
#if PHP_VERSION_ID < 70300
			create.flags = CONST_CS;
			create.module_number = PHP_USER_CONSTANT;
#else
			ZEND_CONSTANT_SET_FLAGS(&create, CONST_CS, PHP_USER_CONSTANT);
#endif
			create.name = zend_string_copy(key);

			zend_register_constant(&create);
		} else {
			zend_declare_class_constant(clazz, 
				ZSTR_VAL(name), ZSTR_LEN(name), variable);
			Z_TRY_ADDREF_P(variable);
		}

		zend_string_release(key);
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
			zend_string_release(key);
			return 0;
		}

	} else {
		zend_hash_del(table, key);

		zend_declare_class_constant(clazz, 
			ZSTR_VAL(name), ZSTR_LEN(name), variable);
		Z_TRY_ADDREF_P(variable);
	}

	zend_string_release(key);
	return 1;
} /* }}} */

/* {{{ */
zend_bool uopz_constant_undefine(zend_class_entry *clazz, zend_string *name) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);
	zend_string *heap = NULL;

	if (!(zconstant = zend_hash_find_ptr(table, name))) {
		if (!clazz) {
			char *ns = zend_memrchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name));
			size_t nss;

			if (ns) {
				zend_string *heap = zend_string_tolower(name);	

				ns++;
				nss =  (ZSTR_VAL(name) + ZSTR_LEN(name)) - ns;
				
				memcpy(&ZSTR_VAL(heap)[ZSTR_LEN(heap) - nss], ns, nss);
				
				zconstant = zend_hash_find_ptr(table, heap);

				if (!zconstant) {
					zend_string_release(heap);
					return 0;
				}

				name = heap;

				goto _uopz_constant_undefine;
			}
		}
		return 0;
	}

_uopz_constant_undefine:
	if (!clazz) {
#if PHP_VERSION_ID < 70300
		if (zconstant->module_number != PHP_USER_CONSTANT) {
#else
		if (ZEND_CONSTANT_MODULE_NUMBER(zconstant) != PHP_USER_CONSTANT) {
#endif
			if (heap) {
				zend_string_release(heap);
			}

			uopz_exception(
				"failed to undefine the internal constant %s, not allowed", ZSTR_VAL(name));
			return 0;
		}

		if (heap) {
			zend_string_release(heap);
		}

		zend_hash_del(table, name);

		return 1;
	}

	if (heap) {
		zend_string_release(heap);
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
