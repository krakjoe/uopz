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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_closures.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_extensions.h"
#include "Zend/zend_string.h"
#include "Zend/zend_vm_opcodes.h"

#ifdef HAVE_SPL
#include "ext/spl/spl_exceptions.h"
#endif

#include "uopz.h"
#include "compile.h"
#include "copy.h"

ZEND_DECLARE_MODULE_GLOBALS(uopz)

/* {{{ */
zend_class_entry *spl_ce_RuntimeException;
zend_class_entry *spl_ce_InvalidArgumentException; /* }}} */

#define MAX_OPCODE 163
#undef EX
#define EX(element) execute_data->element
#define OPLINE EX(opline)
#define OPCODE OPLINE->opcode

#if PHP_VERSION_ID >= 50500
# define EX_T(offset) (*EX_TMP_VAR(execute_data, offset))
#else
# define EX_T(offset) (*(temp_variable *)((char*)execute_data->Ts + offset))
#endif

#define uopz_parse_parameters(spec, ...) zend_parse_parameters_ex\
	(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, spec, ##__VA_ARGS__)
#define uopz_refuse_parameters(message, ...) zend_throw_exception_ex\
	(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, message, ##__VA_ARGS__)
#define uopz_exception(message, ...) zend_throw_exception_ex\
	(spl_ce_RuntimeException, 0 TSRMLS_CC, message, ##__VA_ARGS__)

/* {{{ */
typedef struct _uopz_key_t {
	char       *string;
	zend_uint   length;
	zend_ulong  hash;
	zend_bool   copied;
} uopz_key_t; /* }}} */

static zend_bool uopz_backup(zend_class_entry *scope, uopz_key_t *key TSRMLS_DC);

/* {{{ */
PHP_INI_BEGIN()
	 STD_PHP_INI_ENTRY("uopz.overloads",  "0",    PHP_INI_SYSTEM,    OnUpdateBool,       ini.overloads,          zend_uopz_globals,        uopz_globals)
	 STD_PHP_INI_ENTRY("uopz.backup",     "1",    PHP_INI_SYSTEM,    OnUpdateBool,       ini.backup,             zend_uopz_globals,        uopz_globals)
	 STD_PHP_INI_ENTRY("uopz.fixup",      "0",    PHP_INI_SYSTEM,    OnUpdateBool,       ini.fixup,              zend_uopz_globals,        uopz_globals)
PHP_INI_END() /* }}} */

/* {{{ */
user_opcode_handler_t ohandlers[MAX_OPCODE]; /* }}} */

/* {{{ */
typedef struct _uopz_opcode_t {
	zend_uchar   code;
	const char  *name;
	size_t       length;
	zend_uint    arguments;
	const char  *expected;
} uopz_opcode_t;

#define UOPZ_CODE(n, a, e)   {n, #n, sizeof(#n)-1, a, e}
#define UOPZ_CODE_END  		 {ZEND_NOP, NULL, 0L, 0, NULL}

uopz_opcode_t uoverrides[] = {
	UOPZ_CODE(ZEND_NEW, 1, "function(class)"),
	UOPZ_CODE(ZEND_THROW, 1, "function(exception)"),
	UOPZ_CODE(ZEND_FETCH_CLASS, 1, "function(class)"),
	UOPZ_CODE(ZEND_ADD_TRAIT, 2, "function(class, trait)"),
	UOPZ_CODE(ZEND_ADD_INTERFACE, 2, "function(class, interface)"),
	UOPZ_CODE(ZEND_INSTANCEOF, 2, "function(object, class)"),
	UOPZ_CODE(ZEND_EXIT, 0, "function(void)"),
	UOPZ_CODE_END
}; /* }}} */

/* {{{ */
static inline const uopz_opcode_t* uopz_opcode_find(zend_uchar opcode) {
	uopz_opcode_t *uop = uoverrides;
	
	while (uop->code != ZEND_NOP) {
		if (uop->code == opcode)
			return uop;
		uop++;
	}
	
	return NULL;
} /* }}} */

/* {{{ */
static inline const char* uopz_opcode_name(zend_uchar opcode) {
	const uopz_opcode_t *uop = uopz_opcode_find(opcode);
	
	if (!uop) {
		return "unknown";
	}
	
	return uop->name;
} /* }}} */

/* {{{ */
typedef struct _uopz_magic_t {
	const char *name;
	size_t      length;
	int         id;
} uopz_magic_t;

#define UOPZ_MAGIC(name, id) {name, sizeof(name)-1, id}
#define UOPZ_MAGIC_END	     {NULL, 0, 0L}

uopz_magic_t umagic[] = {
	UOPZ_MAGIC(ZEND_CONSTRUCTOR_FUNC_NAME, 0),
	UOPZ_MAGIC(ZEND_DESTRUCTOR_FUNC_NAME, 1),
	UOPZ_MAGIC(ZEND_CLONE_FUNC_NAME, 2),
	UOPZ_MAGIC(ZEND_GET_FUNC_NAME, 3),
	UOPZ_MAGIC(ZEND_SET_FUNC_NAME, 4),
	UOPZ_MAGIC(ZEND_UNSET_FUNC_NAME, 5),
	UOPZ_MAGIC(ZEND_ISSET_FUNC_NAME, 6),
	UOPZ_MAGIC(ZEND_CALL_FUNC_NAME, 7),
	UOPZ_MAGIC(ZEND_CALLSTATIC_FUNC_NAME, 8),
	UOPZ_MAGIC(ZEND_TOSTRING_FUNC_NAME, 9),
	UOPZ_MAGIC("serialize", 10),
	UOPZ_MAGIC("unserialize", 11),
#ifdef ZEND_DEBUGINFO_FUNC_NAME
	UOPZ_MAGIC(ZEND_DEBUGINFO_FUNC_NAME, 12),
#endif
	UOPZ_MAGIC_END
};
/* }}} */

/* {{{ */
typedef struct _uopz_handler_t {
	zval *handler;
} uopz_handler_t; /* }}} */

/* {{{ */
typedef struct _uopz_backup_t {
	uopz_key_t        name;
	zend_class_entry *scope;
	zend_function     internal;
} uopz_backup_t; /* }}} */

/* {{{ */
static inline zend_bool __uopz_make_key_inline(zval* pzval, uopz_key_t *key, zend_bool copy TSRMLS_DC) {
	key->string = NULL;
	key->length = 0;
	key->copied = 0;

	if (!pzval) {
		goto bad;
	}

	if (Z_STRLEN_P(pzval)) {
		key->string = Z_STRVAL_P(pzval);
		key->length = Z_STRLEN_P(pzval)+1;
		if (copy) {
			key->string = zend_str_tolower_dup
				(key->string, key->length);
			key->copied = copy;
		}
		key->hash = zend_inline_hash_func(key->string, key->length);
		return 1;
	}

bad:
	if (EG(in_execution)) {
		uopz_refuse_parameters(
			"invalid input to function, expected string and got %s", 
			pzval ? zend_zval_type_name(pzval) : "nothin'");
	}

	return 0;
} /*}}} */

#define uopz_make_key_ex(p, k, f) __uopz_make_key_inline(p, k, f TSRMLS_CC)
#define uopz_make_key(p, k) 	  __uopz_make_key_inline(p, k, 1 TSRMLS_CC)

/* {{{ */
static uopz_key_t uopz_copy_key(uopz_key_t *key) {
	uopz_key_t copy = *key;

	if (copy.copied) {
		copy.string = estrndup
			(copy.string, copy.length);
	}

	return copy;
} /* }}} */

/* {{{ */
static void uopz_free_key(uopz_key_t *key) {
	if (key && key->string && key->copied) {
		efree(key->string);
	}
} /* }}} */

/* {{{ this is awkward, but finds private functions ... so don't "fix" it ... */
static int uopz_find_function(HashTable *table, uopz_key_t *name, zend_function **function TSRMLS_DC) {
	HashPosition position;
	zend_function *entry;
	Bucket *bucket = table ? table->pListHead : NULL;
	uopz_key_t lower = *name;

	if (!bucket) {
		return FAILURE;
	}

	if (!lower.copied) {
		lower.copied = 3;
		lower.string = zend_str_tolower_dup(lower.string, lower.length);
		lower.hash = zend_inline_hash_func(lower.string, lower.length);
	}

	while (bucket) {
		if (lower.length == bucket->nKeyLength &&
			lower.hash   == bucket->h &&
			memcmp(lower.string, bucket->arKey, lower.length) == SUCCESS) {
			if (function) {
				*function = (zend_function*) bucket->pData;
			}

			if (lower.copied == 3)
				uopz_free_key(&lower);
			return SUCCESS;
		}
		bucket = bucket->pListNext;
	}

	if (lower.copied == 3)	
		uopz_free_key(&lower);

	return FAILURE;
} /* }}} */

/* {{{ */
static void php_uopz_init_globals(zend_uopz_globals *ng) {
	ng->overload._exit = NULL;
	ng->ini.backup = 1;
	ng->ini.overloads = 0;
	ng->ini.fixup = 0;
} /* }}} */

/* {{{ */
static void php_uopz_handler_dtor(void *pData) {
	uopz_handler_t *uhandler = (uopz_handler_t*) pData;

	if (uhandler->handler) {
		zval_ptr_dtor(&uhandler->handler);
	}
} /* }}} */

/* {{{ */
static void php_uopz_backup_dtor(void *pData) {
	uopz_backup_t *backup = (uopz_backup_t *) pData;
	zend_function *restored = NULL;
	
	TSRMLS_FETCH();

	if (backup->internal.type == ZEND_INTERNAL_FUNCTION) {
		HashTable *table = backup->scope ?
			&backup->scope->function_table :
			CG(function_table);
		
		zend_hash_quick_update(
			table, 
			backup->name.string, backup->name.length, backup->name.hash,
			(void**) &backup->internal, sizeof(zend_function),
			NULL);
	}
	
	if (backup->scope)
		backup->scope->refcount--;
	
	destroy_zend_function(&backup->internal TSRMLS_CC);
	uopz_free_key(&backup->name);
} /* }}} */

/* {{{ */
static void php_uopz_backup_table_dtor(void *pData) {
	zend_hash_destroy((HashTable*) pData);
} /* }}} */

/* {{{ */
static int php_uopz_handler(ZEND_OPCODE_HANDLER_ARGS) {
	uopz_handler_t *uhandler = NULL;
	int dispatching = 0;

	if (zend_hash_index_find(&UOPZ(overload).table, OPCODE, (void**)&uhandler) == SUCCESS) {
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
		char *cerror = NULL;
		zval *retval = NULL;
		zval *op1 = NULL,
			 *op2 = NULL,
			 *result = NULL;
		zend_free_op free_op1, free_op2, free_result;
		zend_class_entry *oce = NULL, **nce = NULL;

		memset(
			&fci, 0, sizeof(zend_fcall_info));

		if (zend_is_callable_ex(uhandler->handler, NULL, IS_CALLABLE_CHECK_SILENT, NULL, NULL, &fcc, &cerror TSRMLS_CC)) {
			if (zend_fcall_info_init(uhandler->handler, 
					IS_CALLABLE_CHECK_SILENT, 
					&fci, &fcc, 
					NULL, &cerror TSRMLS_CC) == SUCCESS) {

				fci.params = (zval***) emalloc(2 * sizeof(zval **));
				fci.param_count = 2;

				switch (OPCODE) {
					case ZEND_INSTANCEOF: {
						GET_OP1(BP_VAR_R);

						oce = EX_T(OPLINE->op2.var).class_entry; 

						MAKE_STD_ZVAL(op2);
						ZVAL_STRINGL(op2, oce->name, oce->name_length, 1);
						fci.params[1] = &op2;
					} break;
					
					case ZEND_FETCH_CLASS: {						
						switch(OPLINE->op2_type) {
							case IS_UNUSED:
								/* allow the vm to do whatever it was doing, I 'unno ... */
								return ZEND_USER_OPCODE_DISPATCH_TO | ZEND_FETCH_CLASS;
							
							default: {
								/* allow the vm to handle exceptions */
								if (UNEXPECTED(EG(exception) != NULL)) {
									return ZEND_USER_OPCODE_DISPATCH_TO | ZEND_FETCH_CLASS;
								}
							}
						}
						
						MAKE_STD_ZVAL(op1);
						ZVAL_STRINGL(op1, Z_STRVAL_P(OPLINE->op2.zv), Z_STRLEN_P(OPLINE->op2.zv), 1);
						fci.params[0] = &op1;
						fci.param_count = 1;
					} break;

					case ZEND_ADD_INTERFACE:
					case ZEND_ADD_TRAIT: {
						oce = EX_T(OPLINE->op1.var).class_entry;

						MAKE_STD_ZVAL(op1);
						ZVAL_STRINGL(op1, oce->name, oce->name_length, 1);
						fci.params[0] = &op1;
						
						MAKE_STD_ZVAL(op2);
						ZVAL_STRINGL(op2, Z_STRVAL_P(OPLINE->op2.zv), Z_STRLEN_P(OPLINE->op2.zv), 1);
						fci.params[1] = &op2;
					} break;

					case ZEND_NEW: {
						oce = EX_T(OPLINE->op1.var).class_entry;

						MAKE_STD_ZVAL(op1);
						ZVAL_STRINGL(op1, oce->name, oce->name_length, 1);
						fci.param_count = 1;
						fci.params[0] = &op1;
					} break;
					
					case ZEND_THROW: {
						GET_OP1(BP_VAR_RW);
						fci.param_count = 1;
					} break;

					default: {
						GET_OP1(BP_VAR_RW);
						GET_OP2(BP_VAR_RW);
					}
				}

				fci.retval_ptr_ptr = &retval;

				zend_try {
					zend_call_function(&fci, &fcc TSRMLS_CC);
				} zend_end_try();

				if (fci.params)
					efree(fci.params);

				if (retval) {
					if (Z_TYPE_P(retval) != IS_NULL) {
						convert_to_long(retval);
						dispatching = 
							Z_LVAL_P(retval);
					}
					zval_ptr_dtor(&retval);
				}

				switch (OPCODE) {
					case ZEND_INSTANCEOF: {
						convert_to_string(op2);

						if (zend_lookup_class(Z_STRVAL_P(op2), Z_STRLEN_P(op2), &nce TSRMLS_CC) == SUCCESS) {
							if (*nce != oce) {
								EX_T(OPLINE->op2.var).class_entry = *nce;
							}
						}

						zval_ptr_dtor(&op2);
					} break;
					
					/* note: fetch requires us to continue explicitly; nobody else gets a shot, caches are ignored
						note: you get what you pay for, but you must pay !! */
					case ZEND_FETCH_CLASS: {
						if (Z_TYPE_P(op1) == IS_OBJECT) {
							EX_T(OPLINE->result.var).class_entry = Z_OBJCE_P(op2);
						} else if (Z_TYPE_P(op1) == IS_STRING) {
							EX_T(OPLINE->result.var).class_entry =  zend_fetch_class(Z_STRVAL_P(op1), Z_STRLEN_P(op1), OPLINE->extended_value TSRMLS_CC);
						} else return (ZEND_USER_OPCODE_DISPATCH_TO | ZEND_FETCH_CLASS);
						
						zval_ptr_dtor(&op1);
						
						if (EX_T(OPLINE->result.var).class_entry) {
							OPLINE++;
							return ZEND_USER_OPCODE_CONTINUE;
						}
					} break;

					case ZEND_ADD_INTERFACE:
					case ZEND_ADD_TRAIT: {
						convert_to_string(op2);

						if (zend_lookup_class(Z_STRVAL_P(op2), Z_STRLEN_P(op2), &nce TSRMLS_CC) == SUCCESS) {
							if (*nce != oce) {
								CACHE_PTR(OPLINE->op2.literal->cache_slot, *nce);
							}
						}

						zval_ptr_dtor(&op1);
						zval_ptr_dtor(&op2);
					} break;

					case ZEND_NEW: {
						convert_to_string(op1);

						if (zend_lookup_class(Z_STRVAL_P(op1), Z_STRLEN_P(op1), &nce TSRMLS_CC) == SUCCESS) {
							if (*nce != oce) {
								EX_T(OPLINE->op1.var).class_entry = *nce;
							}
						}

						zval_ptr_dtor(&op1);
					} break;
					
					case ZEND_THROW: {
						
					} break;
				}
			}
		}
	}
	
	if (dispatching) switch(dispatching) {
		case ZEND_USER_OPCODE_CONTINUE:
			EX(opline)++;

		default:
			return dispatching;
	}

	if (ohandlers[OPCODE]) {
		return ohandlers[OPCODE]
			(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH_TO | OPCODE;
} /* }}} */

/* {{{ */
static inline void php_uopz_backup(TSRMLS_D) {
	HashTable *table = CG(function_table);
	HashPosition position[2];
	zval name;
	uopz_key_t key;

	for (zend_hash_internal_pointer_reset_ex(table, &position[0]);
		 zend_hash_get_current_key_ex(table, &Z_STRVAL(name), &Z_STRLEN(name), NULL, 0, &position[0]) == HASH_KEY_IS_STRING;
		 zend_hash_move_forward_ex(table, &position[0])) {
		 Z_STRLEN(name)--;
		 if (uopz_make_key_ex(&name, &key, 0)) {
		 	uopz_backup(NULL, &key TSRMLS_CC);	
		 }
	}

	table = CG(class_table);

	{
		zend_class_entry **clazz = NULL;

		for (zend_hash_internal_pointer_reset_ex(table, &position[0]);
			 zend_hash_get_current_data_ex(table, (void**) &clazz, &position[0]) == SUCCESS;
			 zend_hash_move_forward_ex(table, &position[0])) {
			 if (!(*clazz)->ce_flags & ZEND_ACC_INTERFACE) {
			 	for (zend_hash_internal_pointer_reset_ex(&(*clazz)->function_table, &position[1]);
			 		 zend_hash_get_current_key_ex(&(*clazz)->function_table, &Z_STRVAL(name), &Z_STRLEN(name), NULL, 0, &position[1]) == HASH_KEY_IS_STRING;
			 		 zend_hash_move_forward_ex(&(*clazz)->function_table, &position[1])) {
			 		 Z_STRLEN(name)--;
			 		 if (uopz_make_key_ex(&name, &key, 0)) {
			 		 	uopz_backup((*clazz), &key TSRMLS_CC);
			 		 }
			 	}
			 }
		}	
	}
} /* }}} */

/* {{{ */
static inline void php_uopz_init_handlers(int module TSRMLS_DC) {
	memset(ohandlers, 0, sizeof(user_opcode_handler_t) * MAX_OPCODE);

	{
#define REGISTER_ZEND_UOPCODE(u) \
	zend_register_long_constant\
		((u)->name, (u)->length+1, (u)->code, CONST_CS|CONST_PERSISTENT, module TSRMLS_CC)

		uopz_opcode_t *uop = uoverrides;

		while (uop->code != ZEND_NOP) {
			zval constant;

			if (UOPZ(ini).overloads) {
				if (uop->code != ZEND_EXIT) {
					ohandlers[uop->code] = 
						zend_get_user_opcode_handler(uop->code);
					zend_set_user_opcode_handler(uop->code, php_uopz_handler);
				}
			}
			
			if (!zend_get_constant(uop->name, uop->length+1, &constant TSRMLS_CC)) {
				REGISTER_ZEND_UOPCODE(uop);
			} else zval_dtor(&constant);

			uop++;
		}

#undef REGISTER_ZEND_UOPCODE
	}
} /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(uopz)
{
	ZEND_INIT_MODULE_GLOBALS(uopz, php_uopz_init_globals, NULL);

	UOPZ(copts) = CG(compiler_options);

	CG(compiler_options) |= 
		(ZEND_COMPILE_HANDLE_OP_ARRAY);

	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_CONTINUE",		ZEND_USER_OPCODE_CONTINUE,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_ENTER",		ZEND_USER_OPCODE_ENTER,			CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_LEAVE", 		ZEND_USER_OPCODE_LEAVE,			CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_DISPATCH", 	ZEND_USER_OPCODE_DISPATCH,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_DISPATCH_TO", 	ZEND_USER_OPCODE_DISPATCH_TO,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_RETURN", 		ZEND_USER_OPCODE_RETURN, 		CONST_CS|CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("ZEND_ACC_PUBLIC", 				ZEND_ACC_PUBLIC, 				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_PRIVATE", 				ZEND_ACC_PRIVATE,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_PROTECTED", 			ZEND_ACC_PROTECTED,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_PPP_MASK", 			ZEND_ACC_PPP_MASK,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_STATIC", 				ZEND_ACC_STATIC,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_FINAL", 				ZEND_ACC_FINAL,					CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_ABSTRACT", 			ZEND_ACC_ABSTRACT,				CONST_CS|CONST_PERSISTENT);

	/* just for consistency */
	REGISTER_LONG_CONSTANT("ZEND_ACC_CLASS",     			0,  							CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_INTERFACE", 			ZEND_ACC_INTERFACE, 			CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_TRAIT",    			ZEND_ACC_TRAIT,     			CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_ACC_FETCH",				LONG_MAX,						CONST_CS|CONST_PERSISTENT);
	
	REGISTER_INI_ENTRIES();

	php_uopz_init_handlers(module_number TSRMLS_CC);

	if (UOPZ(ini).fixup) {
		CG(class_table)->pDestructor = NULL;
		CG(function_table)->pDestructor = NULL;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ */
static PHP_MSHUTDOWN_FUNCTION(uopz)
{
	CG(compiler_options) = UOPZ(copts);

	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(uopz)
{
	zend_class_entry **ce = NULL;

	spl_ce_RuntimeException = 
			(zend_lookup_class(ZEND_STRL("RuntimeException"), &ce TSRMLS_CC) == SUCCESS) ?
				*ce : zend_exception_get_default(TSRMLS_C);
	
	spl_ce_InvalidArgumentException = 
			(zend_lookup_class(ZEND_STRL("InvalidArgumentException"), &ce TSRMLS_CC) == SUCCESS) ?
				*ce : zend_exception_get_default(TSRMLS_C);

	zend_hash_init(
		&UOPZ(overload).table, 8, NULL,
		(dtor_func_t) php_uopz_handler_dtor, 0);	
	zend_hash_init(
		&UOPZ(backup), 8, NULL, 
		(dtor_func_t) php_uopz_backup_table_dtor, 0);

	if (UOPZ(ini).backup) {
		php_uopz_backup(TSRMLS_C);
	}
	
	return SUCCESS;
} /* }}} */

/* {{{ */
static inline int php_uopz_clean_user_function(void *pData TSRMLS_DC) {
	zend_function *function = (zend_function*) pData;

	if (function->type == ZEND_USER_FUNCTION) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ */
static inline int php_uopz_clean_user_class(void *pData TSRMLS_DC) {
	zend_class_entry **ce = (zend_class_entry**) pData;

	zend_hash_apply(
		&(*ce)->function_table, php_uopz_clean_user_function TSRMLS_CC);
	
	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(uopz)
{
	if (UOPZ(overload)._exit) {
		zval_ptr_dtor(&UOPZ(overload)._exit);
	}

	zend_hash_destroy(&UOPZ(overload).table);
	zend_hash_destroy(&UOPZ(backup));

	zend_hash_apply(CG(function_table), php_uopz_clean_user_function TSRMLS_CC);
	zend_hash_apply(CG(class_table), php_uopz_clean_user_class TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(uopz)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "uopz support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ */
static inline void php_uopz_overload_exit(zend_op_array *op_array) {

	zend_uint it = 0;
	zend_uint end = op_array->last;
	zend_op  *opline = NULL;
	TSRMLS_FETCH();

	while (it < end) {
		opline = &op_array->opcodes[it];

		switch (opline->opcode) {
			case ZEND_EXIT: {
				znode     result;
				zval      call;

				ZVAL_STRINGL(&call,
					"__uopz_exit_overload", 
					sizeof("__uopz_exit_overload")-1, 1);

				opline->opcode = ZEND_DO_FCALL;
				SET_NODE(op_array, opline->op1, call);
				SET_UNUSED(opline->op2);
#if PHP_VERSION_ID > 50500
				opline->op2.num = CG(context).nested_calls;
#endif
				opline->result.var = php_uopz_temp_var(op_array TSRMLS_CC);
				opline->result_type = IS_TMP_VAR;
				GET_NODE(op_array, &result, opline->result);		
				CALCULATE_LITERAL_HASH(op_array, opline->op1.constant);
				GET_CACHE_SLOT(op_array, opline->op1.constant);
			} break;
		}

		it++;
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_verify_overload(zval *handler, long opcode, char **expected TSRMLS_DC) {
	const uopz_opcode_t *uop = uopz_opcode_find(opcode);
	zend_fcall_info_cache fcc;
	char *cerror = NULL;
	
	if (!uop) {
		*expected = "a supported opcode";
		return 0;
	}
	
	if (!zend_is_callable_ex(handler, NULL, IS_CALLABLE_CHECK_SILENT, NULL, NULL, &fcc, &cerror TSRMLS_CC) ||
		fcc.function_handler->common.num_args != uop->arguments) {
		*expected = (char*) uop->expected;
		if (cerror) {
			efree(cerror);
		}
		return 0;
	}
	
	return 1;
} /* }}} */

/* {{{ proto bool uopz_overload(int opcode, Callable handler) */
static PHP_FUNCTION(uopz_overload)
{
	zval *handler = NULL;
	long  opcode = ZEND_NOP;
	char *expected = NULL;

	if (uopz_parse_parameters("l|z", &opcode, &handler) != SUCCESS) {
		uopz_refuse_parameters(
				"unexpected parameter combination, expected (int [, Callable])");
		return;
	}
	
	if (!UOPZ(ini).overloads) {
		uopz_exception("overloads are disabled by configuration");
		return;
	}

	if (!handler || Z_TYPE_P(handler) == IS_NULL) {
		if (opcode == ZEND_EXIT) {
			if (UOPZ(overload)._exit) {
				zval_ptr_dtor(&UOPZ(overload)._exit);
			
				UOPZ(overload)._exit = NULL;
			}
		} else {
			zend_hash_index_del(&UOPZ(overload).table, opcode);
		}
		
		RETURN_TRUE;
	}

	if (!uopz_verify_overload(handler, opcode, &expected TSRMLS_CC)) {
		uopz_refuse_parameters(
			"invalid handler for %s, expected %s", 
			uopz_opcode_name(opcode), expected);
		return;
	}

	if (opcode == ZEND_EXIT) {
		if (UOPZ(overload)._exit) {
			zval_ptr_dtor(&UOPZ(overload)._exit);
		}

		MAKE_STD_ZVAL(UOPZ(overload)._exit);
		ZVAL_ZVAL(UOPZ(overload)._exit, handler, 1, 0);
	} else {
		uopz_handler_t uhandler;

		MAKE_STD_ZVAL(uhandler.handler);
		ZVAL_ZVAL(uhandler.handler, handler, 1, 0);
		zend_hash_index_update(
			&UOPZ(overload).table, opcode, (void**) &uhandler, sizeof(uopz_handler_t), NULL);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ */
static zend_bool uopz_backup(zend_class_entry *clazz, uopz_key_t *name TSRMLS_DC) {
	HashTable     *backup = NULL;
	zend_function *function = NULL;
	HashTable     *table = (clazz) ? &clazz->function_table : CG(function_table);

	if (!name->string) {
		return 0;
	}

	if (zend_hash_quick_find(table, name->string, name->length, name->hash, (void**) &function) != SUCCESS) {
		return 0;
	}

	if (zend_hash_index_find(&UOPZ(backup), (zend_ulong) table, (void**) &backup) != SUCCESS) {
		HashTable creating;
		
		zend_hash_init(&creating, 8, NULL, (dtor_func_t) php_uopz_backup_dtor, 0);
		zend_hash_index_update(	
			&UOPZ(backup),
			(zend_ulong) table,
			(void**) &creating,
			sizeof(HashTable), (void**) &backup);
	}

	if (!zend_hash_quick_exists(backup, name->string, name->length, name->hash)) {
		uopz_backup_t ubackup;

		ubackup.name = uopz_copy_key(name);
		ubackup.scope = clazz;
		ubackup.internal = uopz_copy_function(function TSRMLS_CC);

		if (zend_hash_quick_update(
			backup,
			name->string, name->length, name->hash,
			(void**) &ubackup,
			sizeof(uopz_backup_t), NULL) != SUCCESS) {	
			if (clazz) {
				uopz_exception(
					"backup of %s::%s failed, update failed", 
					clazz->name, name->string);
			} else {
				uopz_exception(
					"backup of %s failed, update failed", 
					name->string);
			}
			return 0;
		}

		if (clazz)
			clazz->refcount++;

		return 1;
	}

	return 0;
} /* }}} */

/* {{{ proto bool uopz_backup(string class, string function)
       proto bool uopz_backup(string function) */
PHP_FUNCTION(uopz_backup) {
	uopz_key_t uname;
	zval *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("Cz", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;

		case 1: if (uopz_parse_parameters("z", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
			return;
	}

	if (uopz_make_key(name, &uname)) {
		RETVAL_BOOL(uopz_backup(clazz, &uname TSRMLS_CC));
		uopz_free_key(&uname);
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_restore(zend_class_entry *clazz, uopz_key_t *name TSRMLS_DC) {
	zend_function *function = NULL;
	HashTable     *backup = NULL;
	uopz_backup_t *ubackup = NULL;
	HashTable     *table = clazz ? &clazz->function_table : CG(function_table);

	if (zend_hash_index_find(&UOPZ(backup), (zend_ulong) table, (void**) &backup) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"restoration of %s::%s failed, no backups for the class %s could be found", 
				clazz->name, name->string, clazz->name);
		} else {
			uopz_exception(
				"restoration of %s failed, no backup could be found", 
				name->string);
		}
		return 0;
	}

	if (zend_hash_quick_find(backup, name->string, name->length, name->hash, (void**)&ubackup) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"restoration of %s::%s failed, no backups for the function %s could be found", 
				clazz->name, name->string, name->string);
		} else {
			uopz_exception(
				"restoration of %s failed, no backup for the function could be found", 
				name->string);
		}
		return 0;
	}

	table = ubackup->scope ?
		&ubackup->scope->function_table :
		CG(function_table);

	if (zend_hash_quick_update(
		table, 
		ubackup->name.string, ubackup->name.length, ubackup->name.hash,
		(void**)&ubackup->internal, sizeof(zend_function), (void**)&function) == SUCCESS) {
		function->common.prototype = function;
		function_add_ref(function);
	} else {
		if (clazz) {
			uopz_exception(
				"restoration of %s::%s failed, update failed", 
				clazz->name, name->string, name->string);
		} else {
			uopz_exception(
				"restoration of %s failed, update failed", 
				name->string);
		}
		return 0;
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_restore(string class, string function)
	   proto bool uopz_restore(string function) */
PHP_FUNCTION(uopz_restore) {
	zval *name = NULL;
	zend_class_entry *clazz = NULL;
	uopz_key_t uname;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("Cz", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;

		case 1: if (uopz_parse_parameters("z", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function) expected");
			return;
	}

	if (uopz_make_key(name, &uname)) {
		RETVAL_BOOL(uopz_restore(clazz, &uname TSRMLS_CC));
		uopz_free_key(&uname);
	}
} /* }}} */

/* {{{ */
static inline void uopz_copy(zend_class_entry *clazz, uopz_key_t *name, zval **return_value, zval *this_ptr TSRMLS_DC) {
	HashTable *table = (clazz) ? &clazz->function_table : CG(function_table);
	zend_function *function = NULL, *closure = NULL;
	zend_class_entry *scope = EG(scope);
	zend_bool staticify = 0;
	
	if (!name->string) {
		return;
	}

	if (uopz_find_function(table, name, &function TSRMLS_CC) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"could not find the requested function (%s::%s)", 
				clazz->name, name->string);
		} else {
			uopz_exception("could not find the requested function (%s)", name->string);
		}
		return;
	}

	staticify = function->common.fn_flags & ZEND_ACC_STATIC;
	EG(scope)=function->common.scope; 
	zend_create_closure(
	    *return_value,
	    function, function->common.scope,
	    this_ptr TSRMLS_CC);
	{
		closure = zend_get_closure_method_def(*return_value TSRMLS_CC);
		if (staticify) {
			closure->common.fn_flags |= ZEND_ACC_STATIC;
		} else closure->common.fn_flags &= ~ZEND_ACC_STATIC;
	}
	EG(scope)=scope;
} /* }}} */

/* {{{ proto Closure uopz_copy(string class, string function)
	   proto Closure uopz_copy(string function) */
PHP_FUNCTION(uopz_copy) {
	zval *name = NULL;
	zend_class_entry *clazz = NULL;
	uopz_key_t uname;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("Cz", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;

		case 1: if (uopz_parse_parameters("z", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
			return;
	}

	if (uopz_make_key_ex(name, &uname, 0)) {
		uopz_copy(clazz, &uname, &return_value, EG(This) TSRMLS_CC);
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_rename(zend_class_entry *clazz, uopz_key_t *name, uopz_key_t *rename TSRMLS_DC) {
	zend_function *tuple[2] = {NULL, NULL};
	zend_ulong hashed[2] = {0L, 0L};
	zend_function locals[2];
	zend_bool result = 1;
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);

	if (!name->string && !rename->string) {
		return 0;
	}

	uopz_find_function(table, name, &tuple[0] TSRMLS_CC);
	uopz_find_function(table, rename, &tuple[1] TSRMLS_CC);

	if (!tuple[0] && !tuple[1]) {
		if (clazz) {
			uopz_exception(
				"failed to find the functions %s::%s and %s::%s", 
				clazz->name, name->string, clazz->name, rename->string);
		} else {
			uopz_exception(
				"failed to find the functions %s and %s", 
				name->string, rename->string);
		}
		return 0;
	}

	if (tuple[0] && tuple[1]) {
		locals[0] = *tuple[0];
		locals[1] = *tuple[1];

		if (tuple[0]->type == ZEND_INTERNAL_FUNCTION && 
			tuple[1]->type == ZEND_INTERNAL_FUNCTION) {
			/* both internal */
		} else {
			if (tuple[0]->type == ZEND_INTERNAL_FUNCTION ||
				tuple[1]->type == ZEND_INTERNAL_FUNCTION) {
				/* one internal */
				if (tuple[0]->type == ZEND_INTERNAL_FUNCTION) {
					function_add_ref(&locals[1]);
				} else if (tuple[1]->type == ZEND_INTERNAL_FUNCTION) {
					function_add_ref(&locals[0]);
				}
			} else {
				/* both user */
				function_add_ref(&locals[0]);
				function_add_ref(&locals[1]);
			}
		}

		if (zend_hash_quick_update(table, 
				name->string, name->length, name->hash, 
				(void**) &locals[1], sizeof(zend_function), NULL) != SUCCESS ||
			zend_hash_quick_update(table,
				rename->string, rename->length, rename->hash,
				(void**) &locals[0], sizeof(zend_function), NULL) != SUCCESS) {
			if (clazz) {
				uopz_exception(
					"failed to rename the functions %s::%s and %s::%s, switch failed", 
					clazz->name, name->string, clazz->name, rename->string);
			} else {
				uopz_exception(
					"failed to rename the functions %s and %s, switch failed", 
					name->string, rename->string);
			}
			result = 0;	
		}

		return 1;
		
	}

	/* only one existing function */
	locals[0] = tuple[0] ? *tuple[0] : *tuple[1];

	if (zend_hash_quick_update(table,
		rename->string, rename->length, rename->hash, 
		(void**) &locals[0], sizeof(zend_function), 
		tuple[0] ? (void**)  &tuple[1] : (void**) &tuple[0]) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"failed to rename the function %s::%s to %s::%s, update failed", 
				clazz->name, name->string, clazz->name, rename->string);
		} else {
			uopz_exception(
				"failed to rename the function %s to %s, update failed", 
				name->string, rename->string);	
		}
		return 0;
	}

	function_add_ref(tuple[0] ? tuple[1] : tuple[0]);

	return 1;
} /* }}} */

/* {{{ proto bool uopz_rename(mixed name, mixed rename)
	   proto bool uopz_rename(string class, mixed name, mixed rename) */
PHP_FUNCTION(uopz_rename) {
	zval *name = NULL;
	zval *rename = NULL;
	zend_class_entry *clazz = NULL;
	uopz_key_t uname, urename;
	
	switch (ZEND_NUM_ARGS()) {
		case 3: if (uopz_parse_parameters("Czz", &clazz, &name, &rename) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, name, rename)");
			return;
		} break;
		
		case 2: if (uopz_parse_parameters("zz", &name, &rename) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (name, rename)");
			return;
		} break;
		
		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, name, rename) or (name, rename)");
			return;
	}
	
	if (!uopz_make_key(name, &uname)) {
		return;
	}
	
	if (!uopz_make_key(rename, &urename)) {
		uopz_free_key(&uname);
		return;
	}
	
	RETVAL_BOOL(uopz_rename(clazz, &uname, &urename TSRMLS_CC));
	uopz_free_key(&uname);
	uopz_free_key(&urename);
} /* }}} */

/* {{{ */
static inline zend_bool uopz_delete(zend_class_entry *clazz, uopz_key_t *name TSRMLS_DC) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	uopz_magic_t *magic = umagic;
	
	if (!name->string) {
		return 0;
	}
	
	uopz_backup(clazz, name TSRMLS_CC);

	if (!zend_hash_quick_exists(table, name->string, name->length, name->hash)) {
		if (clazz) {
			uopz_exception(
				"failed to delete the function %s::%s, it does not exist", clazz->name, name->string);
		} else {
			uopz_exception(
				"failed to delete the function %s, it does not exist", name->string);
		}
		return 0;
	}

	if (zend_hash_quick_del(table, name->string, name->length, name->hash) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"failed to delete the function %s::%s, delete failed", clazz->name, name->string);
		} else {
			uopz_exception(
				"failed to delete the function %s, delete failed", name->string);
		}
		return 0;
	}

	if (clazz) {
		while (magic && magic->name) {
			if ((name->length-1) == magic->length &&
				strncasecmp(name->string, magic->name, magic->length) == SUCCESS) {

				switch (magic->id) {
					case 0: clazz->constructor = NULL; break;
					case 1: clazz->destructor = NULL; break;
					case 2: clazz->clone = NULL; break;
					case 3: clazz->__get = NULL; break;
					case 4: clazz->__set = NULL; break;
					case 5: clazz->__unset = NULL; break;
					case 6: clazz->__isset = NULL; break;
					case 7: clazz->__call = NULL; break;
					case 8: clazz->__callstatic = NULL; break;
					case 9: clazz->__tostring = NULL; break;
					case 10: clazz->serialize_func = NULL; break;
					case 11: clazz->unserialize_func = NULL; break;
#ifdef ZEND_DEBUGINFO_FUNC_NAME
					case 12: clazz->__debugInfo = NULL; break;
#endif
				}
				break;
			}
			magic++;
		}	
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_delete(mixed function)
	   proto bool uopz_delete(string class, mixed function) */
PHP_FUNCTION(uopz_delete) {
	uopz_key_t uname;
	zval *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("Cz", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;
		
		case 1: if (uopz_parse_parameters("z", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;
		
		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
			return;
	}

	if (!uopz_make_key(name, &uname)) {
		return;
	}

	RETVAL_BOOL(uopz_delete(clazz, &uname TSRMLS_CC));
	uopz_free_key(&uname);
} /* }}} */

/* {{{ proto void __uopz_exit_overload() */
PHP_FUNCTION(__uopz_exit_overload) {
	zend_bool  leave = 1;

	if(UOPZ(overload)._exit) {
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
		char *cerror = NULL;
		zval *retval = NULL;

		memset(
			&fci, 0, sizeof(zend_fcall_info));
		if (zend_is_callable_ex(UOPZ(overload)._exit, NULL, IS_CALLABLE_CHECK_SILENT, NULL, NULL, &fcc, &cerror TSRMLS_CC)) {
			if (zend_fcall_info_init(UOPZ(overload)._exit, 
					IS_CALLABLE_CHECK_SILENT, 
					&fci, &fcc, 
					NULL, &cerror TSRMLS_CC) == SUCCESS) {

				fci.retval_ptr_ptr = &retval;

				zend_try {
					zend_call_function(&fci, &fcc TSRMLS_CC);
				} zend_end_try();

				if (retval) {
#if PHP_VERSION_ID > 50600
					leave = zend_is_true(retval TSRMLS_CC);
#else
					leave = zend_is_true(retval);
#endif
					zval_ptr_dtor(&retval);
				}
			}
		}
	}

	zval_ptr_dtor(&return_value);

	if (leave) {
		zend_bailout();
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_redefine(zend_class_entry *clazz, uopz_key_t *name, zval *variable TSRMLS_DC) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);

	switch (Z_TYPE_P(variable)) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_STRING:
		case IS_BOOL:
		case IS_RESOURCE:
		case IS_NULL:
			break;

		default:
			if (clazz) {
				uopz_exception(
					"failed to redefine the constant %s::%s, type not allowed", clazz->name, name->length);
			} else {
				uopz_exception(
					"failed to redefine the constant %s, type not allowed", name->length);
			}
			return 0;			
	}

	if (!name->string) {
		return 0;
	}

	if (zend_hash_quick_find(
		table, name->string, name->length, name->hash, (void**)&zconstant) != SUCCESS) {
		if (!clazz) {
			zend_constant create;

			ZVAL_ZVAL(&create.value, variable, 1, 0);
			create.flags = CONST_CS;
			create.name = zend_strndup(name->string, name->length);
			create.name_len = name->length;
			create.module_number = PHP_USER_CONSTANT;

			if (zend_register_constant(&create TSRMLS_CC) != SUCCESS) {
				uopz_exception(
					"failed to redefine the constant %s, operation failed", name->length);
				zval_dtor(&create.value);
				return 0;
			}
		} else {
			zval *create;

			MAKE_STD_ZVAL(create);
			ZVAL_ZVAL(create, variable, 1, 0);
			if (zend_hash_quick_update(
				table, 
				name->string, name->length, name->hash, 
				(void**)&create, sizeof(zval*), NULL) != SUCCESS) {
				uopz_exception(
					"failed to redefine the constant %s::%s, update failed", clazz->name, name->length);
				zval_ptr_dtor(&create);	
				return 0;
			}
		}

		return 1;
	}

	if (!clazz) {
		if (zconstant->module_number == PHP_USER_CONSTANT) {
			zval_dtor(&zconstant->value);
			ZVAL_ZVAL(
				&zconstant->value, variable, 1, 0);
		} else {
			uopz_exception(
				"failed to redefine the internal %s, not allowed", name->length);
			return 0;
		}
	} else {
		zval *copy;

		MAKE_STD_ZVAL(copy);
		ZVAL_ZVAL(copy, variable, 1, 0);
		if (zend_hash_quick_update(
			table, 
			name->string, name->length, name->hash,
			(void**)&copy, sizeof(zval*), NULL) != SUCCESS) {
			uopz_exception(
				"failed to redefine the constant %s::%s, update failed", clazz->name, name->length);
			zval_ptr_dtor(&copy);
			return 0;
		}
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_redefine(string constant, mixed variable)
	   proto bool uopz_redefine(string class, string constant, mixed variable) */
PHP_FUNCTION(uopz_redefine)
{
	uopz_key_t uname;
	zval *name = NULL;
	zval *variable = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 3: {
			if (uopz_parse_parameters("Czz", &clazz, &name, &variable) != SUCCESS) {
				uopz_refuse_parameters(
					"unexpected parameter combination, expected (class, constant, variable)");
				return;
			}
		} break;

		case 2: if (uopz_parse_parameters("zz", &name, &variable) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (constant, variable)");
			return;
		} break;

		default: {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, constant, variable) or (constant, variable)");
			return;
		}
	}

	if (!uopz_make_key_ex(name, &uname, 0)) {
		return;
	}

	if (uopz_redefine(clazz, &uname, variable TSRMLS_CC)) {
		if (clazz) {
			while ((clazz = clazz->parent)) {
				uopz_redefine(
					clazz, &uname, variable TSRMLS_CC);
			}
		}	
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_undefine(zend_class_entry *clazz, uopz_key_t *name TSRMLS_DC) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);

	if (!name->string) {
		return 0;
	}

	if (zend_hash_quick_find(
			table, name->string, name->length, name->hash, (void**)&zconstant) != SUCCESS) {
		return 0;
	}

	if (!clazz) {
		if (zconstant->module_number != PHP_USER_CONSTANT) {
			uopz_exception(
				"failed to undefine the internal constant %s, not allowed", name->string);	
			return 0;
		}

		if (zend_hash_quick_del
			(table, name->string, name->length, name->hash) != SUCCESS) {
			uopz_exception(
				"failed to undefine the constant %s, delete failed", name->string);
			return 0;
		}

		return 1;
	}

	if (zend_hash_quick_del(table, name->string, name->length, name->hash) != SUCCESS) {
		uopz_exception(
			"failed to undefine the constant %s::%s, delete failed", clazz->name, name->length);
		return 0;
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_undefine(string constant) 
	   proto bool uopz_undefine(string class, string constant) */
PHP_FUNCTION(uopz_undefine)
{
	uopz_key_t uname;
	zval *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: {
			if (uopz_parse_parameters("Cz", &clazz, &name) != SUCCESS) {
				uopz_refuse_parameters(
					"unexpected parameter combination, expected (class, constant)");
				return;
			}
		} break;

		case 1: {
			if (uopz_parse_parameters("z", &name) != SUCCESS) {
				uopz_refuse_parameters(
					"unexpected parameter combination, expected (constant)");
				return;
			}
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, constant) or (constant)");
			return;
	}

	if (!uopz_make_key_ex(name, &uname, 0)) {
		return;
	}

	if (uopz_undefine(clazz, &uname TSRMLS_CC)) {
		if (clazz) {
			while ((clazz = clazz->parent)) {
				uopz_undefine(clazz, &uname TSRMLS_CC);
			}
		}
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_function(zend_class_entry *clazz, uopz_key_t *name, zval *closure, long flags TSRMLS_DC) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_function *destination = NULL;
	uopz_key_t lower = *name;
	zend_function *function = (zend_function*) zend_get_closure_method_def(closure TSRMLS_CC);
	
	if (!name->string) {
		return 0;
	}
	
	if (!lower.copied) {
		lower.copied = 2;
		lower.string = zend_str_tolower_dup
			(lower.string, lower.length);
		lower.hash = zend_inline_hash_func(lower.string, lower.length);
	}
	
	uopz_backup(clazz, &lower TSRMLS_CC);

	if (!flags) {
		/* get flags from original function */
		if (uopz_find_function(table, &lower, &destination TSRMLS_CC) == SUCCESS) {
			flags = destination->common.fn_flags;
		} else {
			/* set flags to sensible default */
			flags = ZEND_ACC_PUBLIC;
		}
		
		destination = NULL;
	}

	if (zend_hash_quick_update(
		table, 
		lower.string, lower.length, lower.hash, 
		(void**) function, sizeof(zend_function),
		(void**) &destination) != SUCCESS) {
		uopz_free_key(&lower);
		if (clazz) {
			uopz_exception("failed to create function %s::%s, update failed", clazz->name, name->string);
		} else {
			uopz_exception("failed to create function %s, update failed", name->string);
		}
		return 0;
	}
	
	if (lower.copied == 2) {
		uopz_free_key(&lower);
	}
	
	destination->common.fn_flags = flags;
	destination->common.prototype = destination;
	
	function_add_ref(destination);

	if (clazz) {
		uopz_magic_t *magic = umagic;

		while (magic && magic->name) {
			if ((name->length-1) == magic->length &&
				strncasecmp(name->string, magic->name, magic->length) == SUCCESS) {

				switch (magic->id) {
					case 0: clazz->constructor = destination; break;
					case 1: clazz->destructor = destination; break;
					case 2: clazz->clone = destination; break;
					case 3: clazz->__get = destination; break;
					case 4: clazz->__set = destination; break;
					case 5: clazz->__unset = destination; break;
					case 6: clazz->__isset = destination; break;
					case 7: clazz->__call = destination; break;
					case 8: clazz->__callstatic = destination; break;
					case 9: clazz->__tostring = destination; break;
					case 10: clazz->serialize_func = destination; break;
					case 11: clazz->unserialize_func = destination; break;
#ifdef ZEND_DEBUGINFO_FUNC_NAME
					case 12: clazz->__debugInfo = destination; break;
#endif
				}
			}
			magic++;
		}
		destination->common.scope = clazz;
	} else destination->common.scope = NULL;

	return 1;
} /* }}} */

/* {{{ proto bool uopz_function(string function, Closure handler [, int flags = 0])
	   proto bool uopz_function(string class, string method, Closure handler [, int flags = 0]) */
PHP_FUNCTION(uopz_function) {
	zval *name = NULL;
	uopz_key_t uname;
	zval *closure = NULL;
	zend_class_entry *clazz = NULL;
	long flags = 0;

	if (uopz_parse_parameters("zO|l", &name, &closure, zend_ce_closure, &flags) != SUCCESS &&
		uopz_parse_parameters("CzO|l", &clazz, &name, &closure, zend_ce_closure, &flags) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, "
			"expected "
			"(class, name, closure [, flags]) or (name, closure [, flags])");
		return;
	}

	if (!uopz_make_key(name, &uname)) {
		return;
	}

	RETVAL_BOOL(uopz_function(clazz, &uname, closure, flags TSRMLS_CC));
	uopz_free_key(&uname);
} /* }}} */

/* {{{ */
static inline zend_bool uopz_implement(zend_class_entry *clazz, zend_class_entry *interface TSRMLS_DC) {
	zend_bool is_final = 
		(clazz->ce_flags & ZEND_ACC_FINAL);
	
	if (!interface->ce_flags & ZEND_ACC_INTERFACE) {
		uopz_exception(
			"the class provided (%s) is not an interface", interface->name);
		return 0;
	}

	if (instanceof_function(clazz, interface TSRMLS_CC)) {
		uopz_exception(
			"the class provided (%s) already has the interface interface", clazz->name);
		return 0;
	}

	clazz->ce_flags &= ~ZEND_ACC_FINAL;

	zend_do_implement_interface
		(clazz, interface TSRMLS_CC);

	if (is_final)
		clazz->ce_flags |= ZEND_ACC_FINAL;

	return instanceof_function(clazz, interface TSRMLS_CC);
} /* }}} */

/* {{{ proto bool uopz_implement(string class, string interface) */
PHP_FUNCTION(uopz_implement)
{
	zend_class_entry *clazz = NULL;
	zend_class_entry *interface = NULL;

	if (uopz_parse_parameters("CC", &clazz, &interface) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, interface)");
		return;
	}

	RETURN_BOOL(uopz_implement(clazz, interface TSRMLS_CC));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_extend(zend_class_entry *clazz, zend_class_entry *parent TSRMLS_DC) {
	zend_bool is_final = clazz->ce_flags & ZEND_ACC_FINAL;

	clazz->ce_flags &= ~ZEND_ACC_FINAL;

	if ((clazz->ce_flags & ZEND_ACC_INTERFACE) &&
		!(parent->ce_flags & ZEND_ACC_INTERFACE)) {
		uopz_exception(
		    "%s cannot extend %s, because %s is not an interface", 
		     clazz->name, parent->name, parent->name);
		return 0;
	}

	if (instanceof_function(clazz, parent TSRMLS_CC)) {
		uopz_exception(
			"class %s already extends %s", 
			clazz->name, parent->name);
		return 0;
	}

	if (parent->ce_flags & ZEND_ACC_TRAIT) {
		zend_do_implement_trait(clazz, parent TSRMLS_CC);
	} else zend_do_inheritance(clazz, parent TSRMLS_CC);

	if (parent->ce_flags & ZEND_ACC_TRAIT)
		zend_do_bind_traits(clazz TSRMLS_CC);

	if (is_final)
		clazz->ce_flags |= ZEND_ACC_FINAL;

	return instanceof_function(clazz, parent TSRMLS_CC);
} /* }}} */

/* {{{ proto bool uopz_extend(string class, string parent) */
PHP_FUNCTION(uopz_extend)
{
	zend_class_entry *clazz = NULL;
	zend_class_entry *parent = NULL;

	if (uopz_parse_parameters("CC", &clazz, &parent) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (class, parent)");
		return;
	}

	RETURN_BOOL(uopz_extend(clazz, parent TSRMLS_CC));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_compose(uopz_key_t *name, HashTable *classes, HashTable *methods, HashTable *properties, long flags TSRMLS_DC) {
	HashPosition position[2];
	zend_class_entry *entry = NULL;
	uopz_key_t uname = *name;
	uopz_key_t ukey;
	zval **member = NULL;

	if ((flags & ZEND_ACC_INTERFACE)) {
		if ((properties && zend_hash_num_elements(properties))) {
			uopz_exception(
				"interfaces can not have properties");
			return 0;
		}
	}

	uname.string = zend_str_tolower_dup(name->string, name->length);
	uname.hash   = zend_inline_hash_func(uname.string, uname.length);
	uname.copied = 1;

	if (zend_hash_quick_exists(CG(class_table), uname.string, uname.length, uname.hash)) {
		uopz_exception(
			"cannot compose existing class (%s)", name->string);
		uopz_free_key(&uname);
		return 0;
	}

	entry = (zend_class_entry*) emalloc(sizeof(zend_class_entry));
	entry->name = estrndup(name->string, name->length-1);
	entry->name_length = name->length-1;
	entry->type = ZEND_USER_CLASS;

	zend_initialize_class_data(entry, 1 TSRMLS_CC);

	entry->ce_flags |= flags;

	if (zend_hash_quick_update(
		CG(class_table),
		uname.string, uname.length, uname.hash,
		(void**)&entry, sizeof(zend_class_entry*), NULL) != SUCCESS) {
		uopz_exception(
			"cannot compose class (%s), update failed", name->string);
		uopz_free_key(&uname);
		efree((char*)entry->name);
		efree(entry);
		return 0;	
	}

#define uopz_compose_bail(s, ...) do {\
	uopz_exception(s, ##__VA_ARGS__);\
	zend_hash_quick_del( \
		CG(class_table), \
		uname.string, uname.length, uname.hash); \
	uopz_free_key(&uname);\
	return 0; \
} while(0)

	for (zend_hash_internal_pointer_reset_ex(classes, &position[0]);
		zend_hash_get_current_data_ex(classes, (void**)&member, &position[0]) == SUCCESS;
		zend_hash_move_forward_ex(classes, &position[0])) {
		zend_class_entry **parent = NULL;

		if (Z_TYPE_PP(member) == IS_STRING) {
			if (zend_lookup_class(
					Z_STRVAL_PP(member),
					Z_STRLEN_PP(member), &parent TSRMLS_CC) == SUCCESS) {
				
				if (entry->ce_flags & ZEND_ACC_TRAIT) {
					if ((*parent)->ce_flags & ZEND_ACC_INTERFACE) {
						uopz_compose_bail(
							"trait %s can not implement interface %s, not allowed",
							entry->name, (*parent)->name);
					}
				}
				
				if ((*parent)->ce_flags & ZEND_ACC_INTERFACE) {
					if (entry->ce_flags & ZEND_ACC_INTERFACE) {
						if (!entry->parent) {
							zend_do_inheritance(entry, *parent TSRMLS_CC);
						} else {
							uopz_compose_bail(
								"interface %s may not extend %s, parent of %s already set to %s",
								entry->name, 
								(*parent)->name, 
								entry->name, 
								entry->parent->name);
						}
					} else zend_do_implement_interface(entry, *parent TSRMLS_CC);
				} else if ((*parent)->ce_flags & ZEND_ACC_TRAIT) {
					zend_do_implement_trait(entry, *parent TSRMLS_CC);
				} else {
					if (!entry->parent) {
						zend_do_inheritance(entry, *parent TSRMLS_CC);
					} else {
						uopz_compose_bail(
							"class %s may not extend %s, parent of %s already set to %s",
							entry->name, 
							(*parent)->name, 
							entry->name, 
							entry->parent->name);
					}
				}
			}
		}
	}

	if (methods) {
		for (zend_hash_internal_pointer_reset_ex(methods, &position[0]);
			 zend_hash_get_current_data_ex(methods, (void**)&member, &position[0]) == SUCCESS;
			 zend_hash_move_forward_ex(methods, &position[0])) {
			 zval **closure = NULL;
			 zend_ulong flags = 0;
			 
			 memset(&ukey, 0, sizeof(uopz_key_t));
			 
			 switch (Z_TYPE_PP(member)) {
			 	case IS_ARRAY: 
			 		if (zend_hash_num_elements(Z_ARRVAL_PP(member)) == 1)
			 			break;
			 	
			 	case IS_OBJECT: 
			 		if (instanceof_function(Z_OBJCE_PP(member), zend_ce_closure TSRMLS_CC))
			 			break;
			 	
			 	default:
			 		uopz_compose_bail(
		 				"invalid member found in methods array, expects [modifiers => closure], or closure");
			 }
			 
			 if (zend_hash_get_current_key_ex(methods, &ukey.string, &ukey.length, NULL, 0, &position[0]) != HASH_KEY_IS_STRING) {
			 	uopz_compose_bail(
			 		"invalid key found in methods array, expect string keys to be legal function names");
			 }
			 
			 if (Z_TYPE_PP(member) == IS_ARRAY) {
			 	 zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(member), &position[1]);
				 if ((zend_hash_get_current_key_ex(Z_ARRVAL_PP(member), NULL, NULL, &flags, 0, &position[1]) != HASH_KEY_IS_LONG) ||
				 	(zend_hash_get_current_data_ex(Z_ARRVAL_PP(member), (void**)&closure, &position[1]) != SUCCESS)) {
				 	uopz_compose_bail(
				 		"invalid member found in methods array, "
				 		"expects [int => closure]");
				 }
				 
				 if ((Z_TYPE_PP(closure) != IS_OBJECT) || 
				 	 (!instanceof_function(Z_OBJCE_PP(closure), zend_ce_closure TSRMLS_CC))) {
				 	uopz_compose_bail(
				 		"invalid member found in methods array, "
				 		"expects [int => closure], got [int => other]");
				 }
			 } else {
			 	flags = ZEND_ACC_PUBLIC;
			 	closure = member;
			 }
			 
			 if (!uopz_function(entry, &ukey, *closure, flags TSRMLS_CC)) {
			 	uopz_compose_bail(
			 		"failed to add method %s to class %s, "
			 		"previous exceptions occured", ukey.string, uname.string);
			 }
		}
	}

	if (properties) {
		for (zend_hash_internal_pointer_reset_ex(properties, &position[0]);
			 zend_hash_get_current_data_ex(properties, (void**) &member, &position[0]) == SUCCESS;
			 zend_hash_move_forward_ex(properties, &position[0])) {
			 memset(&ukey, 0, sizeof(uopz_key_t));

			 if ((Z_TYPE_PP(member) != IS_LONG) || 
			 	 (zend_hash_get_current_key_ex(properties, &ukey.string, &ukey.length, &ukey.hash, 0, &position[0]) != HASH_KEY_IS_STRING)) {
			 	uopz_compose_bail(
			 		"invalid member found in properties array, expects [string => int]");
			 }

			 ukey.length--;
			 if (zend_declare_property_null(entry, ukey.string, ukey.length, Z_LVAL_PP(member) TSRMLS_CC) != SUCCESS) {
			 	uopz_compose_bail(
			 		"failed to declare property %s::$%s, engine failure", entry->name, ukey.string);
			 }
		}
	}

	zend_do_bind_traits(entry TSRMLS_CC);
	uopz_free_key(&uname);

	return 1;
} /* }}} */

/* {{{ proto bool uopz_compose(string name, array classes [, array methods [, array properties [, int flags = ZEND_ACC_CLASS]]]) */
PHP_FUNCTION(uopz_compose)
{
	zval *name = NULL;
	HashTable *classes = NULL;
	HashTable *methods = NULL;
	HashTable *properties = NULL;
	long flags = 0;
	uopz_key_t uname;
	
	if (uopz_parse_parameters("zh|hhl", &name, &classes, &methods, &properties, &flags) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (string name, array classes [, array methods [, int flags]])");
		return;
	}

	if (!uopz_make_key_ex(name, &uname, 0)) {
		return;
	}

	RETURN_BOOL(uopz_compose(&uname, classes, methods, properties, flags TSRMLS_CC));
} /* }}} */

/* {{{ */
static inline void uopz_flags(zend_class_entry *clazz, uopz_key_t *name, long flags, zval *return_value TSRMLS_DC) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_function *function = NULL;
	long current = 0;
	
	if (!name->length) {
		if (flags == LONG_MAX) {
			RETURN_LONG(clazz->ce_flags);
		}
		
		if (flags & ZEND_ACC_PPP_MASK) {
			uopz_exception(
				"attempt to set public, private or protected on class entry, not allowed");
			return;	
		}
		
		if (flags & ZEND_ACC_STATIC) {
			uopz_exception(
				"attempt to set static on class entry, not allowed");
			return;
		}
		
		current = clazz->ce_flags;
		clazz->ce_flags = flags;
		RETURN_LONG(current);
	}
	
	if (uopz_find_function(table, name, &function TSRMLS_CC) != SUCCESS) {
		if (clazz) {
			uopz_exception(
			"failed to set or get flags of %s::%s, function does not exist", 
			clazz->name, name->string);
		} else {
			uopz_exception(
				"failed to set or get flags of %s, function does not exist", 
				name->string);
		}
		return;
	}
	
	if (flags == LONG_MAX) {
		RETURN_LONG(function->common.fn_flags);
	}
	
	current = function->common.fn_flags;
	function->common.fn_flags = flags;
	RETURN_LONG(current);	
} /* }}} */

/* {{{ proto int uopz_flags(string function, int flags)
       proto int uopz_flags(string class, string function, int flags) */
PHP_FUNCTION(uopz_flags) 
{
	zval *name = NULL;
	zend_class_entry *clazz = NULL;
	long flags = LONG_MAX;
	uopz_key_t uname;
	
	memset(&uname, 0, sizeof(uopz_key_t));
	
	switch (ZEND_NUM_ARGS()) {
		case 3: if (uopz_parse_parameters("Czl", &clazz, &name, &flags) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected "
				"(string class, string function, int flags)");
			return;
		} break;
		
		case 2: if (uopz_parse_parameters("zl", &name, &flags) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected "
				"(string function, int flags)");
			return;
		} break;
		
		default:	
			uopz_refuse_parameters(
				"unexpected parameter combination, expected "
				"(string class, string function, int flags) or (string function, int flags)");
			return;
	}
	
	if (name && Z_TYPE_P(name) != IS_NULL) {
		if (!uopz_make_key(name, &uname)) {
			return;
		}
	}

	uopz_flags(clazz, &uname, flags, return_value TSRMLS_CC);
	uopz_free_key(&uname);
} /* }}} */

/* {{{ uopz */
ZEND_BEGIN_ARG_INFO(uopz_overload_arginfo, 1)
	ZEND_ARG_INFO(0, opcode)
	ZEND_ARG_INFO(0, callable)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_rename_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
	ZEND_ARG_INFO(0, overload)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_backup_arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_restore_arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_copy__arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_delete_arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_redefine_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, constant)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_undefine_arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, constant)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_function_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
	ZEND_ARG_INFO(0, handler)
	ZEND_ARG_INFO(0, modifiers)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_implement_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, interface)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_extend_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, parent)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_compose_arginfo, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, classes)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_flags_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(__uopz_exit_overload_arginfo, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ uopz_functions[]
 */
static const zend_function_entry uopz_functions[] = {
	PHP_FE(uopz_overload, uopz_overload_arginfo)
	PHP_FE(uopz_backup, uopz_backup_arginfo)
	PHP_FE(uopz_restore, uopz_restore_arginfo)
	PHP_FE(uopz_copy, uopz_copy__arginfo)
	PHP_FE(uopz_rename, uopz_rename_arginfo)
	PHP_FE(uopz_delete, uopz_delete_arginfo)
	PHP_FE(uopz_redefine, uopz_redefine_arginfo)
	PHP_FE(uopz_undefine, uopz_undefine_arginfo)
	PHP_FE(uopz_function, uopz_function_arginfo)
	PHP_FE(uopz_flags, uopz_flags_arginfo)
	PHP_FE(uopz_implement, uopz_implement_arginfo)
	PHP_FE(uopz_extend, uopz_extend_arginfo)
	PHP_FE(uopz_compose, uopz_compose_arginfo)
	/* private function */
	PHP_FE(__uopz_exit_overload, __uopz_exit_overload_arginfo)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ uopz_module_entry
 */
zend_module_entry uopz_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_UOPZ_EXTNAME,
	uopz_functions,
	PHP_MINIT(uopz),
	PHP_MSHUTDOWN(uopz),
	PHP_RINIT(uopz),
	PHP_RSHUTDOWN(uopz),
	PHP_MINFO(uopz),
	PHP_UOPZ_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

static int uopz_zend_startup(zend_extension *extension) /* {{{ */
{	
#if PHP_VERSION_ID >= 50700
	TSRMLS_FETCH();
	
	return zend_startup_module(&uopz_module_entry TSRMLS_CC);
#else
	return zend_startup_module(&uopz_module_entry);
#endif
}
/* }}} */

#ifndef ZEND_EXT_API
#define ZEND_EXT_API    ZEND_DLEXPORT
#endif
ZEND_EXTENSION();

ZEND_EXT_API zend_extension zend_extension_entry = {
	PHP_UOPZ_EXTNAME,
	PHP_UOPZ_VERSION,
	"Joe Watkins <krakjoe@php.net>",
	"https://github.com/krakjoe/uopz",
	"Copyright (c) 2014",
	uopz_zend_startup,
	NULL,           	    /* shutdown_func_t */
	NULL,                   /* activate_func_t */
	NULL,                   /* deactivate_func_t */
	NULL,           	    /* message_handler_func_t */
	php_uopz_overload_exit, /* op_array_handler_func_t */
	NULL, 				    /* statement_handler_func_t */
	NULL,             	    /* fcall_begin_handler_func_t */
	NULL,           	    /* fcall_end_handler_func_t */
	NULL,      			    /* op_array_ctor_func_t */
	NULL,      			    /* op_array_dtor_func_t */
	STANDARD_ZEND_EXTENSION_PROPERTIES
};

#ifdef COMPILE_DL_UOPZ
ZEND_GET_MODULE(uopz)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
