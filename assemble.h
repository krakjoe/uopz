/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2015                                       |
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
#ifndef HAVE_UOPZ_ASSEMBLE_H
#define HAVE_UOPZ_ASSEMBLE_H
/* {{{ */
static inline void uopz_assemble_name(zend_op_array *assembled, zval *disassembly) {
	zval *name = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("name"));
	
	if (!name)
		return;	

	assembled->function_name = zend_string_copy(Z_STR_P(name));
} /* }}} */

/* {{{ */
static inline void uopz_assemble_flag(zend_op_array *assembled, zval *disassembly) {
	zend_string *name = Z_STR_P(disassembly);

	/* this is horrible */
	if (name->len == sizeof("final")-1 && memcmp(name->val, "final", name->len) == SUCCESS) {
		assembled->fn_flags |= ZEND_ACC_FINAL;
	} else if (name->len == sizeof("static")-1 && memcmp(name->val, "static", name->len) == SUCCESS) {
		assembled->fn_flags |= ZEND_ACC_STATIC;
	} else if (name->len == sizeof("reference")-1 && memcmp(name->val, "reference", name->len) == SUCCESS) {
		assembled->fn_flags |= ZEND_ACC_RETURN_REFERENCE;
	} else if (name->len == sizeof("protected")-1 && memcmp(name->val, "protected", name->len) == SUCCESS) {
		assembled->fn_flags |= ZEND_ACC_PROTECTED;
	} else if (name->len == sizeof("private")-1 && memcmp(name->val, "private", name->len) == SUCCESS) {
		assembled->fn_flags |= ZEND_ACC_PRIVATE;
	} else if (name->len == sizeof("public")-1 && memcmp(name->val, "public", name->len) == SUCCESS) {
		assembled->fn_flags |= ZEND_ACC_PUBLIC;
	}
} /* }}} */

/* {{{ */
static inline void uopz_assemble_flags(zend_op_array *assembled, zval *disassembly) {
	zval *flags = zend_hash_str_find(Z_ARRVAL_P(disassembly), ZEND_STRL("flags"));
	zval *flag  = NULL;

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(flags), flag) {
		uopz_assemble_flag(assembled, flag);
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline zend_function* uopz_assemble(zval *disassembly) {
	zend_op_array *assembled = 
		(zend_op_array*) zend_arena_alloc(&CG(arena), sizeof(zend_op_array));

	memset(assembled, 0, sizeof(zend_op_array));

	assembled->type = ZEND_USER_FUNCTION;
	assembled->refcount = (uint32_t*) emalloc(sizeof(uint32_t));
	*(assembled->refcount) = 1;
	
	uopz_assemble_name(assembled, disassembly);
	uopz_assemble_flags(assembled, disassembly);

	return (zend_function*) assembled;
} /* }}} */
#endif
