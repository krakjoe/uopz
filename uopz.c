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
  | Author: Joe Watkins <joe.watkins@live.co.uk>                         |
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
#include "Zend/zend_vm_opcodes.h"

#include "uopz.h"
#include "compile.h"

ZEND_DECLARE_MODULE_GLOBALS(uopz)

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

/* {{{ */
user_opcode_handler_t ohandlers[MAX_OPCODE]; /* }}} */

/* {{{ */
zend_uchar uoverrides[] = {
	ZEND_NEW,
	ZEND_THROW,
	ZEND_ADD_TRAIT,
	ZEND_ADD_INTERFACE,
	ZEND_INSTANCEOF,
	ZEND_NOP
}; /* }}} */

/* {{{ */
typedef struct _uopz_handler_t {
	zval *handler;
	zend_fcall_info_cache *cache;
} uopz_handler_t; /* }}} */

/* {{{ */
static void php_uopz_init_globals(zend_uopz_globals *ng) {
	ng->overload._exit = NULL;
} /* }}} */

/* {{{ */
static void php_uopz_handler_dtor(void *pData) {
	uopz_handler_t *uhandler = (uopz_handler_t*) pData;

	if (uhandler->handler) {
		zval_ptr_dtor(&uhandler->handler);
	}

	if (uhandler->cache) {
		efree(uhandler->cache);
	}
} /* }}} */

/* {{{ */
static void php_uopz_replaced_dtor(void *pData) {
	void **ppData = (void**) pData;
	efree(*ppData);
} /* }}} */

/* {{{ */
static int php_uopz_handler(ZEND_OPCODE_HANDLER_ARGS) {
	uopz_handler_t *uhandler = NULL;
	int dispatching = 0;

	if (zend_hash_index_find(&UOPZ(overload).table, OPCODE, (void**)&uhandler) == SUCCESS) {
		zend_fcall_info fci;
		char *cerror = NULL;
		zval *retval = NULL;
		zval *op1 = NULL,
			 *op2 = NULL,
			 *result = NULL;
		zend_free_op free_op1, free_op2, free_result;
		zend_class_entry *oce = NULL, **nce = NULL;

		memset(
			&fci, 0, sizeof(zend_fcall_info));

		if (zend_fcall_info_init(uhandler->handler, 
				IS_CALLABLE_CHECK_SILENT, 
				&fci, uhandler->cache, 
				NULL, &cerror TSRMLS_CC) == SUCCESS) {

			fci.params = (zval***) emalloc(2 * sizeof(zval **));
			fci.param_count = 2;

#define GET_OP1(as) do {\
	if ((OPLINE->op1_type != IS_UNUSED) &&\
		(op1 = zend_get_zval_ptr(\
			OPLINE->op1_type, &OPLINE->op1, execute_data, &free_op1, as TSRMLS_CC))) {\
		fci.params[0] = &op1;\
	} else {\
		fci.params[0] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)

#define GET_OP2(as) do {\
	if ((OPLINE->op2_type != IS_UNUSED) &&\
		(op2 = zend_get_zval_ptr(\
			OPLINE->op2_type, &OPLINE->op2, execute_data, &free_op2, as TSRMLS_CC))) {\
		fci.params[1] = &op2;\
	} else {\
		fci.params[1] = &EG(uninitialized_zval_ptr);\
	}\
} while(0)

			switch (OPCODE) {
				case ZEND_INSTANCEOF: {
					GET_OP1(BP_VAR_R);

					oce = EX_T(OPLINE->op2.var).class_entry; 

					MAKE_STD_ZVAL(op2);
					ZVAL_STRINGL(op2, oce->name, oce->name_length, 1);
					fci.params[1] = &op2;
				} break;
				
				case ZEND_ADD_INTERFACE:
				case ZEND_ADD_TRAIT: {
					oce = EX_T(OPLINE->op1.var).class_entry;

					MAKE_STD_ZVAL(op1);
					ZVAL_STRINGL(op1, oce->name, oce->name_length, 1);
					fci.params[0] = &op1;
					
					if (CACHED_PTR(OPLINE->op2.literal->cache_slot)) {
						oce = CACHED_PTR(OPLINE->op2.literal->cache_slot);
					} else {
						oce = zend_fetch_class_by_name(
							Z_STRVAL_P(OPLINE->op2.zv), Z_STRLEN_P(OPLINE->op2.zv), 
							OPLINE->op2.literal + 1, ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);
					}

					MAKE_STD_ZVAL(op2);
					ZVAL_STRINGL(op2, oce->name, oce->name_length, 1);
					fci.params[1] = &op2;
					fci.param_count = 2;
				} break;

				case ZEND_NEW: {
					oce = EX_T(OPLINE->op1.var).class_entry;

					MAKE_STD_ZVAL(op1);
					ZVAL_STRINGL(op1, oce->name, oce->name_length, 1);
					fci.param_count = 1;
					fci.params[0] = &op1;
				} break;

				default: {
					GET_OP1(BP_VAR_RW);
					GET_OP2(BP_VAR_RW);
				}
			}
			
			fci.retval_ptr_ptr = &retval;

			zend_try {
				zend_call_function(&fci, uhandler->cache TSRMLS_CC);
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

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(uopz)
{
	ZEND_INIT_MODULE_GLOBALS(uopz, php_uopz_init_globals, NULL);

	UOPZ(copts) = CG(compiler_options);

	CG(compiler_options) |= 
		(ZEND_COMPILE_HANDLE_OP_ARRAY);

	memset(ohandlers, 0, sizeof(user_opcode_handler_t) * MAX_OPCODE);

	{
#define REGISTER_ZEND_OPCODE(name, len, op) \
	zend_register_long_constant\
		(name, len, op, CONST_CS|CONST_PERSISTENT, module_number TSRMLS_CC)

		zend_uchar *opcode = uoverrides;

		REGISTER_ZEND_OPCODE("ZEND_EXIT", sizeof("ZEND_EXIT"), ZEND_EXIT);

		while (*opcode && *opcode != ZEND_NOP) {
			const char *named = NULL;
			size_t named_len = 0L;
			zval constant;

			ohandlers[*opcode] = 
				zend_get_user_opcode_handler(*opcode);
			zend_set_user_opcode_handler(*opcode, php_uopz_handler);

			named = zend_get_opcode_name(*opcode);
			if (named)
				named_len = strlen(named);

			if (named_len) {
				if (!zend_get_constant(named, named_len+1, &constant TSRMLS_CC)) {
					if (named != NULL) {
						REGISTER_ZEND_OPCODE
							(named, named_len+1, *opcode);
					}
				} else zval_dtor(&constant);
			}

			opcode++;
		}

#undef REGISTER_ZEND_OPCODE
	}

	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_CONTINUE", ZEND_USER_OPCODE_CONTINUE, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_ENTER", ZEND_USER_OPCODE_ENTER, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_LEAVE", ZEND_USER_OPCODE_LEAVE, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_DISPATCH", ZEND_USER_OPCODE_DISPATCH, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_DISPATCH_TO", ZEND_USER_OPCODE_DISPATCH_TO, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_USER_OPCODE_RETURN", ZEND_USER_OPCODE_RETURN, CONST_CS|CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ */
static PHP_MSHUTDOWN_FUNCTION(uopz)
{	
	CG(compiler_options) = UOPZ(copts);

	return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(uopz)
{
	zend_hash_init(&UOPZ(overload).table, 8, NULL, (dtor_func_t) php_uopz_handler_dtor, 0);	
	zend_hash_init(&UOPZ(replaced), 8, NULL, (dtor_func_t) php_uopz_replaced_dtor, 0);
	
	return SUCCESS;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(uopz)
{
	if (UOPZ(overload)._exit) {
		zval_ptr_dtor(&UOPZ(overload)._exit);
	}
	
	if (UOPZ(cache)._exit) {
		//efree(UOPZ(cache)._exit);
	}

	zend_hash_destroy(&UOPZ(overload).table);	
	zend_hash_destroy(&UOPZ(replaced));
	
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
	TSRMLS_FETCH();

	zend_uint it = 0,
			  end = op_array->last;
	zend_op  *opline = NULL;

	while (it < end) {
		opline = &op_array->opcodes[it];

		switch (opline->opcode) {
			case ZEND_EXIT: {
				znode    *result = (znode*) emalloc(sizeof(znode));
				zval      call;

				ZVAL_STRINGL(&call,
					"__uopz_exit_overload", 
					sizeof("__uopz_exit_overload")-1, 1);

				opline->opcode = ZEND_DO_FCALL;
				SET_NODE(op_array, opline->op1, call);
				SET_UNUSED(opline->op2);
				opline->op2.num = CG(context).nested_calls;
				opline->result.var = php_uopz_temp_var(op_array TSRMLS_CC);
				opline->result_type = IS_TMP_VAR;
				GET_NODE(op_array, result, opline->result);		
				CALCULATE_LITERAL_HASH(op_array, opline->op1.constant);
				GET_CACHE_SLOT(op_array, opline->op1.constant);
				zend_hash_next_index_insert(
					&UOPZ(replaced), (void**) &result, sizeof(void*), NULL);
			} break;
		}

		it++;
	}
} /* }}} */

/* {{{ proto void uopz_overload(int opcode, Callable handler) */
static PHP_FUNCTION(uopz_overload)
{
	zval *handler = NULL;
	long  opcode = ZEND_NOP;
	zend_fcall_info_cache fcc;
	char *cerror = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|z", &opcode, &handler) != SUCCESS) {
		return;
	}

	if (!handler || Z_TYPE_P(handler) == IS_NULL) {
		if (opcode == ZEND_EXIT) {
			if (UOPZ(overload)._exit) {
				zval_ptr_dtor(&UOPZ(overload)._exit);
				efree(UOPZ(cache)._exit);
			
				UOPZ(overload)._exit = NULL;
				UOPZ(cache)._exit   = NULL;
			}
		} else {
			zend_hash_index_del(&UOPZ(overload).table, opcode);
		}
		
		RETURN_TRUE;
	}

	if (zend_is_callable_ex(handler, NULL, IS_CALLABLE_CHECK_SILENT, NULL, NULL, &fcc, &cerror TSRMLS_CC)) {
		if (opcode == ZEND_EXIT) {
			if (UOPZ(overload)._exit) {
				zval_ptr_dtor(&UOPZ(overload)._exit);
				efree(UOPZ(cache)._exit);
			}

			MAKE_STD_ZVAL(UOPZ(overload)._exit);
			ZVAL_ZVAL(UOPZ(overload)._exit, handler, 1, 0);
			UOPZ(cache)._exit = (zend_fcall_info_cache*) 
				emalloc(sizeof(zend_fcall_info_cache));
			memcpy(UOPZ(cache)._exit, &fcc, sizeof(zend_fcall_info_cache));
		} else {
			uopz_handler_t uhandler;

			MAKE_STD_ZVAL(uhandler.handler);
			ZVAL_ZVAL(uhandler.handler, handler, 1, 0);
			uhandler.cache = (zend_fcall_info_cache*) 
				emalloc(sizeof(zend_fcall_info_cache));
			memcpy(uhandler.cache, &fcc, sizeof(zend_fcall_info_cache));
			zend_hash_index_update(
				&UOPZ(overload).table, opcode, (void**) &uhandler, sizeof(uopz_handler_t), NULL);
		}

		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ */
static inline void uopz_rename(HashTable *table, zval *function, zval *overload TSRMLS_DC) {
	zend_function *tuple[2] = {NULL, NULL};
	zend_ulong hashed[2] = {0L, 0L};
	
	if ((function && (Z_TYPE_P(function) == IS_STRING)) &&
		(overload && (Z_TYPE_P(overload) == IS_STRING))) {
		
		hashed[0] = zend_inline_hash_func(Z_STRVAL_P(function), Z_STRLEN_P(function)+1);
		hashed[1] = zend_inline_hash_func(Z_STRVAL_P(overload), Z_STRLEN_P(overload)+1);
		
		zend_hash_quick_find(table, Z_STRVAL_P(function), Z_STRLEN_P(function)+1, hashed[0], (void**) &tuple[0]);
		zend_hash_quick_find(table, Z_STRVAL_P(overload), Z_STRLEN_P(overload)+1, hashed[1], (void**) &tuple[1]);
		
		if (tuple[0]){
			zend_function store[2];
			
			store[0] = *tuple[0];
			if (tuple[1]) {
				store[1] = *tuple[1];
			}
			
			if (tuple[0]->type == ZEND_USER_FUNCTION) {
				function_add_ref(&store[0]);
			}
			
			if (tuple[1]) {
				if (tuple[1]->type == ZEND_USER_FUNCTION) {
					function_add_ref(&store[1]);
				}
				zend_hash_quick_del(table, Z_STRVAL_P(overload), Z_STRLEN_P(overload)+1, hashed[1]);
			}
			zend_hash_quick_del(table, Z_STRVAL_P(function), Z_STRLEN_P(function)+1, hashed[0]);
			
			zend_hash_quick_update(table, 
				Z_STRVAL_P(overload), Z_STRLEN_P(overload)+1, hashed[1], 
				(void**) &store[0], sizeof(zend_function), NULL);

			if (tuple[1]) {
				zend_hash_quick_update(table, 
					Z_STRVAL_P(function), Z_STRLEN_P(function)+1, hashed[0], 
					(void**) &store[1], sizeof(zend_function), NULL);
			}
		}
	}
} /* }}} */

/* {{{ proto void uopz_rename(mixed function, mixed overload)
	   proto void uopz_rename(string class, mixed function, mixed overload) */
PHP_FUNCTION(uopz_rename) {
	zval *function = NULL;
	zval *overload = NULL;
	zend_class_entry *clazz = NULL;
	HashTable *table = EG(function_table);
	
	switch (ZEND_NUM_ARGS()) {
		case 3: if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Czz", &clazz, &function, &overload) != SUCCESS) {
			return;
		} else {
			table = &clazz->function_table;
		} break;
		
		case 2: if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &function, &overload) != SUCCESS) {
			return;
		} break;
	}
	
	uopz_rename(table, function, overload TSRMLS_CC);
} /* }}} */

/* {{{ */
static inline void uopz_alias(HashTable *table, zval *function, zval *alias TSRMLS_DC) {
	zend_function *tuple[2] = {NULL, NULL};
	zend_ulong hashed[2] = {0L, 0L};
	
	if ((function && (Z_TYPE_P(function) == IS_STRING)) &&
		(alias && (Z_TYPE_P(alias) == IS_STRING))) {
		
		hashed[0] = zend_inline_hash_func(Z_STRVAL_P(function), Z_STRLEN_P(function)+1);
		hashed[1] = zend_inline_hash_func(Z_STRVAL_P(alias), Z_STRLEN_P(alias)+1);
		
		zend_hash_quick_find(table, Z_STRVAL_P(function), Z_STRLEN_P(function)+1, hashed[0], (void**) &tuple[0]);
		zend_hash_quick_find(table, Z_STRVAL_P(alias), Z_STRLEN_P(alias)+1, hashed[1], (void**) &tuple[1]);
		
		if (tuple[0] && !tuple[1]) {
			zend_hash_quick_update(
				table,
				Z_STRVAL_P(alias), Z_STRLEN_P(alias)+1, 
				hashed[1], (void**) tuple[0], sizeof(zend_function), 
				NULL);
			
			if (tuple[0]->type == ZEND_USER_FUNCTION) {
				function_add_ref(tuple[0]);
			}
		}
	}
} /* }}} */

/* {{{ proto void uopz_alias(mixed function, mixed alias)
	   proto void uopz_alias(string class, mixed function, mixed alias) */
PHP_FUNCTION(uopz_alias) {
	zval *function = NULL;
	zval *alias = NULL;
	zend_class_entry *clazz = NULL;
	HashTable *table = EG(function_table);
	
	switch (ZEND_NUM_ARGS()) {
		case 3: if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Czz", &clazz, &function, &alias) != SUCCESS) {
			return;
		} else {
			table = &clazz->function_table;
		} break;
		
		case 2: if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &function, &alias) != SUCCESS) {
			return;
		} break;
	}
	
	uopz_alias(table, function, alias TSRMLS_CC);
} /* }}} */

/* {{{ */
static inline void uopz_delete(HashTable *table, zval *function TSRMLS_DC) {
	if (function && (Z_TYPE_P(function) == IS_STRING)) {
		zend_hash_del(
			table,
			Z_STRVAL_P(function), Z_STRLEN_P(function)+1);
	}
} /* }}} */

/* {{{ proto void uopz_delete(mixed function)
	   proto void uopz_delete(string class, mixed function) */
PHP_FUNCTION(uopz_delete) {
	zval *function = NULL;
	zend_class_entry *clazz = NULL;
	HashTable *table = EG(function_table);
	
	switch (ZEND_NUM_ARGS()) {
		case 2: if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Cz", &clazz, &function) != SUCCESS) {
			return;
		} else {
			table = &clazz->function_table;
		} break;
		
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &function) != SUCCESS) {
			return;
		} break;
	}
	
	uopz_delete(table, function TSRMLS_CC);
} /* }}} */

/* {{{ proto void __uopz_exit_overload() */
PHP_FUNCTION(__uopz_exit_overload) {
	zend_bool  leave = 1;

	if(UOPZ(overload)._exit) {
		zend_fcall_info fci;
		char *cerror = NULL;
		zval *retval = NULL;

		memset(
			&fci, 0, sizeof(zend_fcall_info));

		if (zend_fcall_info_init(UOPZ(overload)._exit, 
				IS_CALLABLE_CHECK_SILENT, 
				&fci, UOPZ(cache)._exit, 
				NULL, &cerror TSRMLS_CC) == SUCCESS) {

			fci.retval_ptr_ptr = &retval;

			zend_try {
				zend_call_function(&fci, UOPZ(cache)._exit TSRMLS_CC);
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

	zval_ptr_dtor(&return_value);

	if (leave) {
		zend_bailout();
	}
} /* }}} */

/* {{{ */
static inline void uopz_redefine(HashTable *table, zval *constant, zval *variable TSRMLS_DC) {
	zend_constant *zconstant;
	zend_ulong     hash = zend_inline_hash_func(Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1);
	
	switch (Z_TYPE_P(variable)) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_STRING:
		case IS_BOOL:
		case IS_RESOURCE:
		case IS_NULL:
			break;
			
		default:
			return;			
	}
	
	if (Z_TYPE_P(constant) == IS_STRING) {
		if (zend_hash_quick_find(
			table, Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1, 
			hash, (void**)&zconstant) == SUCCESS) {
			if ((table == EG(zend_constants))) {
				if (zconstant->module_number == PHP_USER_CONSTANT) {
					zval_dtor(&zconstant->value);
					ZVAL_ZVAL(
						&zconstant->value, variable, 1, 0);
				}
			} else {
				zval *copy;
			
				MAKE_STD_ZVAL(copy);
				ZVAL_ZVAL(copy, variable, 1, 0);
				zend_hash_quick_update(
					table, 
					Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1, 
					hash, (void**)&copy,
					sizeof(zval*), NULL);
			}		
		} else {
			if (table == EG(zend_constants)) {
				zend_constant create;
				
				ZVAL_ZVAL(&create.value, variable, 1, 0);
				create.flags = CONST_CS;
				create.name = zend_strndup(Z_STRVAL_P(constant), Z_STRLEN_P(constant));
				create.name_len = Z_STRLEN_P(constant)+1;
				create.module_number = PHP_USER_CONSTANT;
				
				zend_register_constant(&create TSRMLS_CC);
			} else {
				zval *create;
				
				MAKE_STD_ZVAL(create);
				ZVAL_ZVAL(create, variable, 1, 0);
				zend_hash_quick_update(
					table, 
					Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1, 
					hash, (void**)&create,
					sizeof(zval*), NULL);
			}
		}
	}
} /* }}} */

/* {{{ proto void uopz_redefine(string constant, mixed variable)
	   proto void uopz_redefine(string class, string constant, mixed variable) */
PHP_FUNCTION(uopz_redefine)
{
	zval *constant = NULL;
	zval *variable = NULL;
	HashTable *table = EG(zend_constants);
	zend_class_entry *clazz = NULL;
	
	switch (ZEND_NUM_ARGS()) {
		case 3: {
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Czz", &clazz, &constant, &variable) != SUCCESS) {
				return;
			} else {
				table = &clazz->constants_table;
			}
		} break;
		
		default: {
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &constant, &variable) != SUCCESS) {
				return;
			}
		}
	}
	
	uopz_redefine(table, constant, variable TSRMLS_CC);
	
	if (table != EG(zend_constants)) {
		while ((clazz = clazz->parent)) {
			uopz_redefine(
				&clazz->constants_table, constant, variable TSRMLS_CC);
		}
	}
} /* }}} */

/* {{{ */
static inline void uopz_undefine(HashTable *table, zval *constant TSRMLS_DC) {
	zend_constant *zconstant;
	zend_ulong     hash = zend_inline_hash_func(Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1);
	
	if (Z_TYPE_P(constant) == IS_STRING && zend_hash_quick_find(
			table,
			Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1, hash, (void**)&zconstant) == SUCCESS) {
		if (table == EG(zend_constants)) {
			if (zconstant->module_number == PHP_USER_CONSTANT) {
				zend_hash_quick_del
					(table, Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1, hash);
			}
		} else {
			zend_hash_quick_del(table, Z_STRVAL_P(constant), Z_STRLEN_P(constant)+1, hash);
		}
	}
} /* }}} */

/* {{{ proto void uopz_undefine(string constant) 
	   proto void uopz_undefine(string class, string constant) */
PHP_FUNCTION(uopz_undefine)
{
	zval *constant = NULL;
	HashTable *table = EG(zend_constants);
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: {
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Cz", &clazz, &constant) != SUCCESS) {
				return;
			} else {
				table = &clazz->constants_table;
			}
		} break;
		
		default: {
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &constant) != SUCCESS) {
				return;
			}
		}
	}

	uopz_undefine(table, constant TSRMLS_CC);

	if (table != EG(zend_constants)) {
		while ((clazz = clazz->parent)) {
			uopz_undefine(&clazz->constants_table, constant TSRMLS_CC);
		}
	}
} /* }}} */

/* {{{ */
static inline void uopz_override(HashTable *table, zval *function, zend_function *override TSRMLS_DC) {
	if (Z_TYPE_P(function) == IS_STRING) {
		zend_hash_update(
			table, 
			Z_STRVAL_P(function), Z_STRLEN_P(function)+1, 
			(void**) override, sizeof(zend_function), 
			NULL);
		
		function_add_ref(override);
	}
} /* }}} */

/* {{{ proto void uopz_override(string function, Closure handler) */
PHP_FUNCTION(uopz_override) {
	zval *function = NULL;
	zval *callable = NULL;
	HashTable *table = CG(function_table);
	zend_class_entry *clazz = NULL;
	
	switch (ZEND_NUM_ARGS()) {
		case 3: {
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Czo", &clazz, &function, &callable, zend_ce_closure) != SUCCESS) {
				return;
			} else {
				table = &clazz->function_table;
			}
		} break;
		
		default: {
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zo", &function, &callable, zend_ce_closure) != SUCCESS) {
				return;
			}
		}
	}
	
	uopz_override(table, function, (zend_function*) zend_get_closure_method_def(callable TSRMLS_CC) TSRMLS_CC);
} /* }}} */

/* {{{ proto void uopz_implement(string class, string interface) */
PHP_FUNCTION(uopz_implement)
{
	zend_class_entry *clazz = NULL;
	zend_class_entry *interface = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "CC", &clazz, &interface) != SUCCESS) {
		return;
	}
	
	if (clazz && interface) {
		zend_bool is_final = clazz->ce_flags;

		clazz->ce_flags &= ~ZEND_ACC_FINAL;

		if (!instanceof_function(clazz, interface TSRMLS_CC)) {
			if (interface->ce_flags & ZEND_ACC_INTERFACE) {
				zend_do_implement_interface(clazz, interface TSRMLS_CC);
			}
		}
		
		if (is_final)
			clazz->ce_flags |= ZEND_ACC_FINAL;
	}
} /* }}} */

/* {{{ proto void uopz_extend(string class, string parent) */
PHP_FUNCTION(uopz_extend)
{
	zend_class_entry *clazz = NULL;
	zend_class_entry *parent = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "CC", &clazz, &parent) != SUCCESS) {
		return;
	}
	
	if (clazz && parent) {
		zend_bool is_final = clazz->ce_flags;

		clazz->ce_flags &= ~ZEND_ACC_FINAL;

		if (!instanceof_function(clazz, parent TSRMLS_CC)) {
			if (!(parent->ce_flags & ZEND_ACC_INTERFACE)) {
				if (parent->ce_flags & ZEND_ACC_TRAIT) {
					zend_do_implement_trait(clazz, parent TSRMLS_CC);
				} else zend_do_inheritance(clazz, parent TSRMLS_CC);
			}
		}
		
		if (parent->ce_flags & ZEND_ACC_TRAIT)
			zend_do_bind_traits(clazz TSRMLS_CC);
		
		if (is_final)
			clazz->ce_flags |= ZEND_ACC_FINAL;
	}
} /* }}} */

/* {{{ proto void uopz_compose(string name, array classes) */
PHP_FUNCTION(uopz_compose)
{
	char *class_name = NULL;
	size_t class_name_len = 0L;
	char *lc_class_name = NULL;
	zend_ulong lc_class_hash = 0L;
	zend_class_entry *entry = NULL;
	HashTable *classes = NULL;
	zval **clazz = NULL;
	HashPosition position;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sh", &class_name, &class_name_len, &classes) != SUCCESS) {
		return;
	}

	if (zend_lookup_class(class_name, class_name_len+1, (zend_class_entry***) &entry TSRMLS_CC) != SUCCESS) {
		entry = (zend_class_entry*) emalloc(sizeof(zend_class_entry));
		entry->name = estrndup(class_name, class_name_len);
		entry->name_length = class_name_len;
		entry->type = ZEND_USER_CLASS;
		zend_initialize_class_data(entry, 1 TSRMLS_CC);

		lc_class_name = zend_str_tolower_dup(class_name, class_name_len);
		lc_class_hash = zend_inline_hash_func(lc_class_name, class_name_len+1);

		zend_hash_quick_update(
			CG(class_table),
			lc_class_name, class_name_len+1, lc_class_hash,
			(void**)&entry, sizeof(zend_class_entry*), NULL);

		for (zend_hash_internal_pointer_reset_ex(classes, &position);
			zend_hash_get_current_data_ex(classes, (void**)&clazz, &position) == SUCCESS;
			zend_hash_move_forward_ex(classes, &position)) {
			zend_class_entry **parent = NULL;

			if (Z_TYPE_PP(clazz) == IS_STRING) {
				if (zend_lookup_class(
						Z_STRVAL_PP(clazz),
						Z_STRLEN_PP(clazz), &parent TSRMLS_CC) == SUCCESS) {

					if ((*parent)->ce_flags & ZEND_ACC_INTERFACE) {
						zend_do_implement_interface(entry, *parent TSRMLS_CC);
					} else if ((*parent)->ce_flags & ZEND_ACC_TRAIT) {
						zend_do_implement_trait(entry, *parent TSRMLS_CC);
					} else zend_do_inheritance(entry, *parent TSRMLS_CC);
				}
			}
		}
		
		zend_do_bind_traits(entry TSRMLS_CC);
		efree(lc_class_name);
	}
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
ZEND_BEGIN_ARG_INFO(uopz_alias_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
	ZEND_ARG_INFO(0, alias)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_delete_arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_redefine_arginfo, 2)
	ZEND_ARG_INFO(0, constant)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_undefine_arginfo, 2)
	ZEND_ARG_INFO(0, constant)
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
ZEND_BEGIN_ARG_INFO(__uopz_exit_overload_arginfo, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_override_arginfo, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ uopz_functions[]
 */
static const zend_function_entry uopz_functions[] = {
	PHP_FE(uopz_overload, uopz_overload_arginfo)
	PHP_FE(uopz_rename, uopz_rename_arginfo)
	PHP_FE(uopz_alias, uopz_alias_arginfo)
	PHP_FE(uopz_delete, uopz_delete_arginfo)
	PHP_FE(uopz_redefine, uopz_redefine_arginfo)
	PHP_FE(uopz_undefine, uopz_undefine_arginfo)
	PHP_FE(uopz_override, uopz_override_arginfo)
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
	return zend_startup_module(&uopz_module_entry);
}
/* }}} */

#ifndef ZEND_EXT_API
#define ZEND_EXT_API    ZEND_DLEXPORT
#endif
ZEND_EXTENSION();

zend_extension zend_extension_entry = {
	PHP_UOPZ_EXTNAME,
	PHP_UOPZ_VERSION,
	"Joe Watkins <joe.watkins@live.co.uk>",
	"https://github.com/krakjoe/uopz",
	"Copyright (c) 2014 Joe Watkins",
	uopz_zend_startup,
	NULL,           	    /* shutdown_func_t */
	NULL,           	    /* activate_func_t */
	NULL,           	    /* deactivate_func_t */
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
