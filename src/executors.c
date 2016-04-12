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

#ifndef UOPZ_EXECUTORS
#define UOPZ_EXECUTORS

#include "php.h"
#include "uopz.h"

#include "executors.h"

ZEND_EXTERN_MODULE_GLOBALS(uopz);

typedef void (*zend_execute_internal_f) (zend_execute_data *, zval *);
typedef void (*zend_execute_f) (zend_execute_data *);

void php_uopz_execute_internal(zend_execute_data *execute_data, zval *return_value);
void php_uopz_execute(zend_execute_data *execute_data);

zend_execute_internal_f zend_execute_internal_function;
zend_execute_f zend_execute_function;

void php_uopz_execute_internal(zend_execute_data *execute_data, zval *return_value);
void php_uopz_execute(zend_execute_data *execute_data);

void uopz_executors_init(void) { /* {{{ */
	zend_execute_internal_function = zend_execute_internal;
	zend_execute_internal = php_uopz_execute_internal;
	zend_execute_function = zend_execute_ex;
	zend_execute_ex = php_uopz_execute;
} /* }}} */

void uopz_executors_shutdown(void) { /* {{{ */
	zend_execute_internal = zend_execute_internal_function;
	zend_execute_ex = zend_execute_function;
} /* }}} */

void php_uopz_execute_internal(zend_execute_data *execute_data, zval *return_value) { /* {{{ */
	if (zend_execute_internal_function) {
		zend_execute_internal_function(execute_data, return_value);
	} else execute_internal(execute_data, return_value);
} /* }}} */

void php_uopz_execute(zend_execute_data *execute_data) { /* {{{ */
	if (zend_execute_function) {
		zend_execute_function(execute_data);
	} else execute_ex(execute_data);
} /* }}} */

#endif	/* UOPZ_HANDLERS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
