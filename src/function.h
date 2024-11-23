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

#ifndef UOPZ_FUNCTION_H
#define UOPZ_FUNCTION_H

#define ZEND_ACC_UOPZ (1<<30)

#include "zend.h"

zend_bool uopz_add_function(zend_class_entry *clazz, zend_string *name, zval *closure, zend_long flags, zend_bool all);
zend_bool uopz_del_function(zend_class_entry *clazz, zend_string *name, zend_bool all);

void uopz_flags(zend_class_entry *clazz, zend_string *name, zend_long flags, zval *return_value);
zend_bool uopz_set_static(zend_class_entry *clazz, zend_string *function, zval *statics);
zend_bool uopz_get_static(zend_class_entry *clazz, zend_string *function, zval *return_value);
#ifndef ZEND_EXIT
void ZEND_FASTCALL uopz_exit_function(INTERNAL_FUNCTION_PARAMETERS);
#endif

#endif	/* UOPZ_FUNCTION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
