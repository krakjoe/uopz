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

#ifndef UOPZ_RETURN_H
#define UOPZ_RETURN_H

typedef struct _uopz_return_t {
	zval value;
	zend_uchar flags;
	zend_class_entry *clazz;
	zend_string *function;
} uopz_return_t;

#define UOPZ_RETURN_EXECUTE 0x00000001
#define UOPZ_RETURN_BUSY	0x00000010

#define UOPZ_RETURN_IS_EXECUTABLE(u) (((u)->flags & UOPZ_RETURN_EXECUTE) == UOPZ_RETURN_EXECUTE)
#define UOPZ_RETURN_IS_BUSY(u) (((u)->flags & UOPZ_RETURN_BUSY) == UOPZ_RETURN_BUSY)

zend_bool uopz_set_return(zend_class_entry *clazz, zend_string *name, zval *value, zend_bool execute);
zend_bool uopz_unset_return(zend_class_entry *clazz, zend_string *function);
void uopz_get_return(zend_class_entry *clazz, zend_string *function, zval *return_value);

uopz_return_t* uopz_find_return(zend_function *function);
void uopz_execute_return(uopz_return_t *ureturn, zend_execute_data *execute_data, zval *return_value);

void uopz_return_free(zval *zv);

#endif	/* UOPZ_RETURN_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
