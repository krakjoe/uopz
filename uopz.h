/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2012 - 2014                                |
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

#define PHP_UOPZ_VERSION "1.0.3"
#define PHP_UOPZ_EXTNAME "uopz"

ZEND_BEGIN_MODULE_GLOBALS(uopz)
	struct {
		zval			  *_exit;
		HashTable         table;
	} overload;
	
	struct {
		zend_bool          backup;
	} ini;
	
	zend_ulong			   copts;
	HashTable              backup;
ZEND_END_MODULE_GLOBALS(uopz)

#ifdef ZTS
#define UOPZ(v) TSRMG(uopz_globals_id, zend_uopz_globals *, v)
#else
#define UOPZ(v) (uopz_globals.v)
#endif

#endif	/* UOPZ_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
