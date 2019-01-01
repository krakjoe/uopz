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

#ifndef UOPZ_CLASS_H
#define UOPZ_CLASS_H

void uopz_mock_init(void);
void uopz_set_mock(zend_string *clazz, zval *mock);
void uopz_unset_mock(zend_string *clazz);

zend_bool uopz_extend(zend_class_entry *clazz, zend_class_entry *parent);
zend_bool uopz_implement(zend_class_entry *clazz, zend_class_entry *interface);
int uopz_get_mock(zend_string *clazz, zval *return_value);
int uopz_find_mock(zend_string *clazz, zend_class_entry **mock);

void uopz_set_property(zval *object, zval *member, zval *value);
void uopz_get_property(zval *object, zval *member, zval *value);

void uopz_set_static_property(zend_class_entry *ce, zend_string *property, zval *value);
void uopz_get_static_property(zend_class_entry *ce, zend_string *property, zval *value);

#endif	/* UOPZ_CLASS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
