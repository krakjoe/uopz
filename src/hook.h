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

#ifndef UOPZ_HOOK_H
#define UOPZ_HOOK_H

typedef struct _uopz_hook_t {
	zval closure;
	zend_class_entry *clazz;
	zend_string *function;
	zend_bool busy;
} uopz_hook_t;

zend_bool uopz_set_hook(zend_class_entry *clazz, zend_string *name, zval *closure);
zend_bool uopz_unset_hook(zend_class_entry *clazz, zend_string *function);
void uopz_get_hook(zend_class_entry *clazz, zend_string *function, zval *return_value);

uopz_hook_t* uopz_find_hook(zend_function *function);
void uopz_execute_hook(uopz_hook_t *uhook, zend_execute_data *execute_data);

void uopz_hook_free(zval *zv);
#endif	/* UOPZ_HOOK_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
