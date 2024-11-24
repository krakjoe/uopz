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

#ifndef UOPZ_UTIL_H
#define UOPZ_UTIL_H

extern PHP_FUNCTION(uopz_call_user_func);
extern PHP_FUNCTION(uopz_call_user_func_array);
#ifndef ZEND_EXIT
extern PHP_FUNCTION(uopz_exit);
#endif

void uopz_handle_magic(zend_class_entry *clazz, zend_string *name, zend_function *function);
zend_function *uopz_find_function(HashTable *table, zend_string *name);
zend_function *uopz_find_method(zend_class_entry *ce, zend_string *name);

zend_bool uopz_is_magic_method(zend_class_entry *clazz, zend_string *function);

int uopz_clean_function(zval *zv);
int uopz_clean_class(zval *zv);

void uopz_request_init(void);
void uopz_request_shutdown(void);

static inline void uopz_zval_dtor(zval *zv) { /* {{{ */
	zval_ptr_dtor(zv);
} /* }}} */

static inline zend_bool uopz_is_cuf(zend_execute_data *execute_data) {
	if (EX(func)->type == ZEND_INTERNAL_FUNCTION) {
		if (EX(func)->internal_function.handler == zif_uopz_call_user_func) {
			return 1;
		}
	}
	return 0;
}

static inline zend_bool uopz_is_cufa(zend_execute_data *execute_data) {
	if (EX(func)->type == ZEND_INTERNAL_FUNCTION) {
		if (EX(func)->internal_function.handler == zif_uopz_call_user_func_array) {
			return 1;
		}
	}
	return 0;	
}

#endif	/* UOPZ_UTIL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
