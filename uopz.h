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

#ifndef UOPZ_H
#define UOPZ_H

extern zend_module_entry uopz_module_entry;
#define phpext_uopz_ptr &uopz_module_entry

#define PHP_UOPZ_VERSION "6.1.1-dev"
#define PHP_UOPZ_EXTNAME "uopz"

ZEND_BEGIN_MODULE_GLOBALS(uopz)
	zend_long	copts;

	HashTable   functions;
	HashTable	returns;
	HashTable	mocks;
	HashTable   hooks;

	zend_bool	exit;
	zval 		estatus;
	zend_bool   disable;
ZEND_END_MODULE_GLOBALS(uopz)

#ifdef ZTS
#define UOPZ(v) TSRMG(uopz_globals_id, zend_uopz_globals *, v)
#else
#define UOPZ(v) (uopz_globals.v)
#endif

#include "ext/spl/spl_exceptions.h"
#include "Zend/zend_inheritance.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_closures.h"

#define uopz_exception(message, ...) zend_throw_exception_ex\
	(spl_ce_RuntimeException, 0, message, ##__VA_ARGS__)

#endif	/* UOPZ_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
