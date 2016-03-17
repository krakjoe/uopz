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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_closures.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_string.h"
#include "Zend/zend_inheritance.h"
#include "Zend/zend_compile.h"
#include "Zend/zend_vm_opcodes.h"

#ifdef HAVE_SPL
#include "ext/spl/spl_exceptions.h"
#else
/* {{{ */
zend_class_entry *spl_ce_RuntimeException;
zend_class_entry *spl_ce_InvalidArgumentException; /* }}} */
#endif

#include "uopz.h"
#include "copy.h"

ZEND_DECLARE_MODULE_GLOBALS(uopz)

#define MAX_OPCODE 163
#undef EX
#define EX(element) EG(current_execute_data)->element
#define OPLINE EX(opline)
#define OPCODE OPLINE->opcode

#define ZEND_VM_GET_OP1_IN(as, i) do {\
	if ((OPLINE->op1_type != IS_UNUSED) &&\
		(op1 = zend_get_zval_ptr(\
			OPLINE->op1_type, &OPLINE->op1, execute_data, &free_op1, as TSRMLS_CC))) {\
		ZVAL_COPY(&fci.params[i], op1); \
	} else { \
		ZVAL_COPY(&fci.params[i], &EG(uninitialized_zval)); \
	}\
} while(0)
#define ZEND_VM_GET_OP1(as) ZEND_VM_GET_OP1_IN(as, 0)

#define ZEND_VM_GET_OP2_IN(as, i) do {\
	if ((OPLINE->op2_type != IS_UNUSED) &&\
		(op2 = zend_get_zval_ptr(\
			OPLINE->op2_type, &OPLINE->op2, execute_data, &free_op2, as TSRMLS_CC))) {\
		ZVAL_COPY(&fci.params[i], op2); \
	} else {\
		ZVAL_COPY(&fci.params[i], &EG(uninitialized_zval)); \
	}\
} while(0)
#define ZEND_VM_GET_OP2(as) ZEND_VM_GET_OP2_IN(as, 1)

#define ZEND_VM_CONTINUE()         return 0

#define HANDLE_EXCEPTION() \
	EX(opline) = OPLINE; \
	ZEND_VM_CONTINUE()

#define ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION() \
	OPLINE = EX(opline) + 1; \
	ZEND_VM_CONTINUE()

#define ZEND_VM_SET_OPCODE(new_op) \
	OPLINE = new_op

#define ZEND_VM_JMP(new_op) \
	if (EXPECTED(!EG(exception))) { \
		ZEND_VM_SET_OPCODE(new_op); \
	} \
	ZEND_VM_CONTINUE()

#define uopz_parse_parameters(spec, ...) zend_parse_parameters_ex\
	(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), spec, ##__VA_ARGS__)
#define uopz_refuse_parameters(message, ...) zend_throw_exception_ex\
	(spl_ce_InvalidArgumentException, 0, message, ##__VA_ARGS__)
#define uopz_exception(message, ...) zend_throw_exception_ex\
	(spl_ce_RuntimeException, 0, message, ##__VA_ARGS__)

#define PHP_UOPZ_OVERRIDE 0x2000000

/* {{{ */
PHP_INI_BEGIN()
	 STD_PHP_INI_ENTRY("uopz.overloads",  "0",    PHP_INI_SYSTEM,    OnUpdateBool,       ini.overloads,          zend_uopz_globals,        uopz_globals)
PHP_INI_END() /* }}} */

/* {{{ */
user_opcode_handler_t ohandlers[MAX_OPCODE]; /* }}} */

/* {{{ */
typedef struct _uopz_opcode_t {
	zend_uchar   code;
	const char  *name;
	size_t       length;
	uint32_t    arguments;
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
	UOPZ_CODE(ZEND_EXIT, 1, "function(status)"),
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
	UOPZ_MAGIC(ZEND_DEBUGINFO_FUNC_NAME, 12),
	UOPZ_MAGIC_END
};
/* }}} */

typedef struct _uopz_backup_t {
	HashTable *table;
	zend_string *name;
	zend_function *function;
} uopz_backup_t;

/* {{{ this is awkward, but finds private functions ... so don't "fix" it ... */
static int uopz_find_function(HashTable *table, zend_string *name, zend_function **function) {
	Bucket *bucket;

	ZEND_HASH_FOREACH_BUCKET(table, bucket) {
		if (zend_string_equals_ci(bucket->key, name)) {
			if (function) {
				*function = (zend_function*) Z_PTR(bucket->val);
				return SUCCESS;
			}
		}
	} ZEND_HASH_FOREACH_END();

	return FAILURE;
} /* }}} */

/* {{{ */
static void uopz_backup_table_dtor(zval *zv) {
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_replace_function(HashTable *table, zend_string *name, zend_function *function, zend_bool is_shutdown) {
	zend_bool result;
	dtor_func_t dtor;
	zend_bool keep_current;
	zend_function *current = zend_hash_find_ptr(table, name);

	if (function == current) {
		/* nothing to do actually */
		return 1;
	}

	keep_current = 0;

	if (current && !is_shutdown) {
		/* see, if we have a backed up function and if it matches one being replaced, and if so - don't touch it */
		HashTable *backups = zend_hash_index_find_ptr(&UOPZ(backup), (zend_long) table);
		if (backups) {
			uopz_backup_t *backup = zend_hash_find_ptr(backups, name);
			keep_current = backup && backup->function == current;
		}
	}
	/* in shutdown passed replacement is really a backed up function, so no need to keep current */

	if (keep_current) {
		dtor = table->pDestructor;
		table->pDestructor = NULL;
	}

	if (function) {
		result = zend_hash_update_ptr(table, name, function) != NULL;
	} else {
		result = zend_hash_del(table, name) == SUCCESS;
	}

	if (keep_current) {
		table->pDestructor = dtor;
	}

	return result;
} /* }}} */

/* {{{ */
static void uopz_backup_dtor(zval *zv) {
	uopz_backup_t *backup = (uopz_backup_t*) Z_PTR_P(zv);
	uopz_replace_function(backup->table, backup->name, backup->function, 1);
	zend_string_release(backup->name);
	efree(backup);
} /* }}} */

/* {{{ */
static zend_bool uopz_backup(zend_class_entry *clazz, zend_string *name) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	HashTable *backups = zend_hash_index_find_ptr(&UOPZ(backup), (zend_long) table);
	uopz_backup_t backup;
	zend_function *function;
	zend_string *lower = zend_string_tolower(name);

	if (!backups) {
		ALLOC_HASHTABLE(backups);
		zend_hash_init(backups, 8, NULL, uopz_backup_dtor, 0);
		if (!zend_hash_index_add_ptr(
			&UOPZ(backup), (zend_long) table, backups)) {
			zend_hash_destroy(backups);
			FREE_HASHTABLE(backups);
			return 0;
		}
	}

	if (zend_hash_exists(backups, lower)) {
		return 0;
	} else if (zend_hash_exists(backups, lower)) {
		zend_string_release(lower);
		return 0;
	}

	backup.table = table;
	backup.name  = lower;
	backup.function = uopz_find_function(table, lower, &function) == SUCCESS ? function : NULL;

	if (!zend_hash_add_mem(backups, lower, &backup, sizeof(uopz_backup_t))) {
		zend_string_release(lower);
		return 0;
	}
	return 1;
} /* }}} */

/* {{{ proto bool uopz_backup(string function)
	   proto bool uopz_backup(string class, string method) */
PHP_FUNCTION(uopz_backup) {
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;
	
	if (uopz_parse_parameters("S", &name) != SUCCESS &&
		uopz_parse_parameters("CS", &clazz, &name) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, "
			"expected "
			"(class, name) or (name)");
		return;
	}

	RETVAL_BOOL(uopz_backup(clazz, name));
} /* }}} */

/* {{{ */
static void php_uopz_init_globals(zend_uopz_globals *ng) {
	ng->ini.overloads = 0;
} /* }}} */

/* {{{ */
static void php_uopz_table_dtor(zval *zv) {
	zend_hash_destroy((HashTable*) Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
} /* }}} */

/* {{{ */
static inline void php_uopz_function_dtor(zval *zv) {
	destroy_op_array((zend_op_array*) Z_PTR_P(zv));
} /* }}} */

static int php_uopz_handler(ZEND_OPCODE_HANDLER_ARGS) {
	zval *uhandler = NULL;
	int dispatching = 0;
	zend_execute_data *execute_data = EG(current_execute_data);

	if ((uhandler = zend_hash_index_find(&UOPZ(overload), OPCODE))) {
		
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
		char *cerror = NULL;
		zval retval;
		zval *op1 = &EG(uninitialized_zval),
		     *op2 = &EG(uninitialized_zval);
		zend_free_op free_op1, free_op2;
		zend_class_entry *oce = NULL, *nce = NULL;

		ZVAL_UNDEF(&retval);

		memset(&fci, 0, sizeof(zend_fcall_info));

		if (zend_is_callable_ex(uhandler, Z_OBJ_P(uhandler), IS_CALLABLE_CHECK_SILENT, NULL, &fcc, &cerror)) {
			if (zend_fcall_info_init(uhandler,
					IS_CALLABLE_CHECK_SILENT,
					&fci, &fcc,
					NULL, &cerror) == SUCCESS) {
				
				fci.params = (zval*) ecalloc(2, sizeof(zval));
				ZVAL_UNDEF(&fci.params[0]);
				ZVAL_UNDEF(&fci.params[1]);
				fci.param_count = 2;
				fci.no_separation = 0;

				switch (OPCODE) {
					case ZEND_INSTANCEOF: {
						ZEND_VM_GET_OP1(BP_VAR_R);

						if (OPLINE->op2_type == IS_CONST) {
							oce = CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op2)));
							if (!oce) {
								oce = zend_fetch_class_by_name(
									Z_STR_P(EX_CONSTANT(OPLINE->op2)),
									EX_CONSTANT(OPLINE->op2) + 1,
									ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION
								);
								if (!oce) {
									ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
								}
							}
						} else {
							oce = Z_CE_P(EX_VAR(OPLINE->op2.var));
						}
						ZVAL_STR(&fci.params[1], zend_string_copy(oce->name));
					} break;

					case ZEND_ADD_INTERFACE:
					case ZEND_ADD_TRAIT: {
						oce = Z_CE_P(EX_VAR(OPLINE->op1.var));
						
						if (oce) {
							ZVAL_STR(&fci.params[0], zend_string_copy(oce->name));
						}
						oce = CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op2)));
						
						if (!oce) {
							oce = zend_fetch_class_by_name(Z_STR_P(EX_CONSTANT(OPLINE->op2)),
                                 EX_CONSTANT(OPLINE->op2) + 1,
                                 ZEND_FETCH_CLASS_TRAIT);

							if (!oce) {
								ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
							}
						}

						ZVAL_STR(&fci.params[1], zend_string_copy(oce->name));
					} break;

					case ZEND_NEW: {
						if (OPLINE->op1_type == IS_CONST) {
							ZVAL_COPY(&fci.params[0], EX_CONSTANT(OPLINE->op1));
						} else {
							oce = Z_CE_P(EX_VAR(OPLINE->op1.var));
							ZVAL_STR(&fci.params[0], zend_string_copy(oce->name));
						}
						
						fci.param_count = 1;
					} break;

					case ZEND_FETCH_CLASS: {
						if (OPLINE->op2_type == IS_UNUSED) {
							oce = zend_fetch_class(NULL, OPLINE->extended_value);

							if (!oce) {
								ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
							}
						} else {
							zval *name = EX_CONSTANT(OPLINE->op2);
							if (OPLINE->op2_type == IS_CONST) {
								ZVAL_COPY(&fci.params[0], name);
							} else if (Z_TYPE_P(name) == IS_OBJECT) {
								oce = Z_OBJCE_P(name);
								ZVAL_STR(&fci.params[0], zend_string_copy(oce->name));
							} else if (Z_TYPE_P(name) == IS_STRING) {
								oce = zend_fetch_class_by_name(Z_STR_P(name), name+1, 
									ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
								if (!oce) {
									ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
								}
								ZVAL_STR(&fci.params[0], zend_string_copy(oce->name));
							} else {
								if (EG(exception)) {
									HANDLE_EXCEPTION();
								}
							}
						}
					} break;

					case ZEND_EXIT: {
						ZEND_VM_GET_OP1(BP_VAR_RW);
						fci.param_count = 1;
					} break;

					default: {
						ZEND_VM_GET_OP1(BP_VAR_RW);
						ZEND_VM_GET_OP2(BP_VAR_RW);
					}
				}

				fci.retval = &retval;

				zend_try {
					zend_call_function(&fci, &fcc);
				} zend_end_try();

				if (Z_TYPE(retval) != IS_UNDEF) {
					convert_to_long(&retval);
					dispatching = Z_LVAL(retval);
					zval_ptr_dtor(&retval);
				}

#define CLEANUP_PARAMS() do { \
	if (Z_TYPE(fci.params[0]) != IS_UNDEF) \
		zval_ptr_dtor(&fci.params[0]); \
	if (Z_TYPE(fci.params[1]) != IS_UNDEF) \
		zval_ptr_dtor(&fci.params[1]); \
	if (fci.params) \
		efree(fci.params); \
} while(0)

				switch (OPCODE) {
					case ZEND_INSTANCEOF: {
						convert_to_string(&fci.params[1]);

						nce = zend_lookup_class(Z_STR(fci.params[1]));

						if (nce != oce) {
							if (OPLINE->op2_type == IS_CONST)
								CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op2)), nce);
						}
					} break;

					case ZEND_ADD_INTERFACE:
					case ZEND_ADD_TRAIT: {
						convert_to_string(&fci.params[1]);

						nce = zend_lookup_class(Z_STR(fci.params[1]));
	
						if (nce != oce) {
							CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op2)), nce);
						}
					} break;

					case ZEND_NEW: {
						convert_to_string(&fci.params[0]);
						
						nce = zend_lookup_class(Z_STR(fci.params[0]));

						if (nce != oce) {
							if (OPLINE->op1_type == IS_CONST)
								CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op1)), nce);
						}
					} break;

					case ZEND_FETCH_CLASS: {
						convert_to_string(&fci.params[0]);
						
						nce = zend_lookup_class(Z_STR(fci.params[0]));

						if (nce != oce) {
							if (OPLINE->op2_type != IS_UNUSED)
								CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op2)), nce);
						}
					} break;

					case ZEND_EXIT: {
						if (dispatching == ZEND_USER_OPCODE_CONTINUE) {
							if (EX(opline) < &EX(func)->op_array.opcodes[EX(func)->op_array.last - 1]) {
								ZEND_VM_JMP(OPLINE + 1);
								CLEANUP_PARAMS();
								return ZEND_USER_OPCODE_CONTINUE;
							}
						}

						CLEANUP_PARAMS();
						return ZEND_USER_OPCODE_RETURN;
					} break;
				}

				CLEANUP_PARAMS();
			}
		}
	}

	if (ohandlers[OPCODE]) {
		//return ohandlers[OPCODE]
		//	(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	return ZEND_USER_OPCODE_DISPATCH;
}

/* {{{ */
static inline void php_uopz_init_handlers(int module) {
	memset(ohandlers, 0, sizeof(user_opcode_handler_t) * MAX_OPCODE);

#define REGISTER_ZEND_UOPCODE(u) \
	zend_register_long_constant\
		((u)->name, (u)->length, (u)->code, CONST_CS|CONST_PERSISTENT, module)

	{
		uopz_opcode_t *uop = uoverrides;

		while (uop->code != ZEND_NOP) {
			zval *constant;
			zend_string *name = zend_string_init(uop->name, uop->length, 0);

			if (UOPZ(ini).overloads) {
				ohandlers[uop->code] =
					zend_get_user_opcode_handler(uop->code);
				zend_set_user_opcode_handler(uop->code, php_uopz_handler);
			}

			if (!(constant = zend_get_constant(name))) {
				REGISTER_ZEND_UOPCODE(uop);
			} else zval_ptr_dtor(constant);

			zend_string_release(name);
			uop++;
		}
	}

#undef REGISTER_ZEND_UOPCODE
} /* }}} */

typedef void (*zend_execute_internal_f) (zend_execute_data *, zval *);
typedef void (*zend_execute_f) (zend_execute_data *);

zend_execute_internal_f zend_execute_internal_function;
zend_execute_f zend_execute_function;

static void php_uopz_execute_internal(zend_execute_data *execute_data, zval *return_value) {
	if (zend_execute_internal_function) {
		zend_execute_internal_function(execute_data, return_value);
	} else execute_internal(execute_data, return_value);
}

static void php_uopz_execute(zend_execute_data *execute_data) {
	if (zend_execute_function) {
		zend_execute_function(execute_data);
	} else execute_ex(execute_data);
}

/* {{{ init call hooks */
static int uopz_init_call_hook(ZEND_OPCODE_HANDLER_ARGS) {
	switch (OPCODE) {
		case ZEND_INIT_FCALL_BY_NAME:
		case ZEND_INIT_FCALL:
		case ZEND_INIT_NS_FCALL_BY_NAME: {
			zval *function_name = EX_CONSTANT(OPLINE->op2);
			CACHE_PTR(Z_CACHE_SLOT_P(function_name), NULL);
		} break;

		case ZEND_INIT_METHOD_CALL: {
			if (OPLINE->op2_type == IS_CONST) {
				zval *function_name = EX_CONSTANT(OPLINE->op2);
				CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(function_name), NULL, NULL);
			}
		} break;

		case ZEND_INIT_STATIC_METHOD_CALL: {
			if (OPLINE->op2_type == IS_CONST) {
				zval *function_name = EX_CONSTANT(OPLINE->op2);
				if (OPLINE->op1_type == IS_CONST) {
					CACHE_PTR(Z_CACHE_SLOT_P(function_name), NULL);
				} else {
					CACHE_POLYMORPHIC_PTR(Z_CACHE_SLOT_P(function_name), NULL, NULL);
				}
			}
		} break;
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

static inline void uopz_register_init_call_hooks() {
	zend_set_user_opcode_handler(ZEND_INIT_FCALL_BY_NAME, uopz_init_call_hook);
	zend_set_user_opcode_handler(ZEND_INIT_FCALL, uopz_init_call_hook);
	zend_set_user_opcode_handler(ZEND_INIT_NS_FCALL_BY_NAME, uopz_init_call_hook);
	zend_set_user_opcode_handler(ZEND_INIT_METHOD_CALL, uopz_init_call_hook);
	zend_set_user_opcode_handler(ZEND_INIT_STATIC_METHOD_CALL, uopz_init_call_hook);
} /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(uopz)
{
	ZEND_INIT_MODULE_GLOBALS(uopz, php_uopz_init_globals, NULL);

	//zend_execute_internal_function = zend_execute_internal;
	//zend_execute_internal = php_uopz_execute_internal;
	//zend_execute_function = zend_execute_ex;
	//zend_execute_ex = php_uopz_execute;

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

	REGISTER_LONG_CONSTANT("ZEND_USER_FUNCTION",			ZEND_USER_FUNCTION,				CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("ZEND_INTERNAL_FUNCTION",		ZEND_INTERNAL_FUNCTION,			CONST_CS|CONST_PERSISTENT);

	REGISTER_INI_ENTRIES();

	if (UOPZ(ini).overloads) {
		php_uopz_init_handlers(module_number);
	}

	uopz_register_init_call_hooks();

	return SUCCESS;
}
/* }}} */

/* {{{ */
static PHP_MSHUTDOWN_FUNCTION(uopz)
{
	UNREGISTER_INI_ENTRIES();

	//zend_execute_internal = zend_execute_internal_function;
	//zend_execute_ex = zend_execute_function;

	return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(uopz)
{
	zend_class_entry *ce = NULL;
	zend_string *spl;

#ifdef ZTS
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	spl = zend_string_init(ZEND_STRL("RuntimeException"), 0);
	spl_ce_RuntimeException =
			(ce = zend_lookup_class(spl)) ?
				ce : zend_exception_get_default();
	zend_string_release(spl);

	spl = zend_string_init(ZEND_STRL("InvalidArgumentException"), 0);
	spl_ce_InvalidArgumentException =
			(ce = zend_lookup_class(spl)) ?
				ce : zend_exception_get_default();
	zend_string_release(spl);

	UOPZ(copts) = CG(compiler_options);

	CG(compiler_options) |= ZEND_COMPILE_HANDLE_OP_ARRAY | 
							ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION | 
							ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS | 
							ZEND_COMPILE_IGNORE_USER_FUNCTIONS | 
							ZEND_COMPILE_GUARDS;
	/*
		We are hacking, horribly ... we can just ignore leaks ...
	*/
	PG(report_memleaks)=0;

	zend_hash_init(&UOPZ(overload), 8, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_init(&UOPZ(backup), 8, NULL, uopz_backup_table_dtor, 0);

	return SUCCESS;
} /* }}} */

/* {{{ */
static inline int php_uopz_destroy_user_function(zval *zv) {
	zend_function *function = Z_PTR_P(zv);

	if (function->type == ZEND_USER_FUNCTION) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ */
static inline int php_uopz_destroy_user_functions(zval *zv) {
	zend_class_entry *ce = Z_PTR_P(zv);

	zend_hash_apply(&ce->function_table, php_uopz_destroy_user_function);

	if (ce->type == ZEND_USER_CLASS) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(uopz)
{
	CG(compiler_options) = UOPZ(copts);

	zend_hash_destroy(&UOPZ(overload));

	zend_hash_destroy(&UOPZ(backup));

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(uopz)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "uopz support", "enabled");
	if (UOPZ(ini).overloads)
		php_info_print_table_header(2, "uopz overloads", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ */
static inline zend_bool uopz_verify_overload(zval *handler, zend_long opcode, char **expected) {
	const uopz_opcode_t *uop = uopz_opcode_find(opcode);
	zend_fcall_info_cache fcc;
	char *cerror = NULL;

	if (!uop) {
		*expected = "a supported opcode";
		return 0;
	}

	if (!zend_is_callable_ex(handler, NULL, IS_CALLABLE_CHECK_SILENT, NULL, &fcc, &cerror) ||
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
	zend_long  opcode = ZEND_NOP;
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
		zend_hash_index_del(
			&UOPZ(overload), opcode);
		RETURN_TRUE;
	}

	if (!uopz_verify_overload(handler, opcode, &expected)) {
		uopz_refuse_parameters(
			"invalid handler for %s, expected %s",
			uopz_opcode_name(opcode), expected);
		return;
	}

	zend_hash_index_update(
		&UOPZ(overload), opcode, handler);
	Z_ADDREF_P(handler);

	RETURN_TRUE;
}
/* }}} */

/* {{{ */
static inline void uopz_copy(zend_class_entry *clazz, zend_string *name, zval **return_value, zval *this_ptr) {
	HashTable *table = (clazz) ? &clazz->function_table : CG(function_table);
	zend_function *function = NULL, *closure = NULL;
	zend_class_entry *scope = EG(scope);
	zend_bool staticify = 0;

	if (uopz_find_function(table, name, &function) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"could not find the requested function (%s::%s)",
				ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		} else {
			uopz_exception("could not find the requested function (%s)", ZSTR_VAL(name));
		}
		return;
	}

	staticify = function->common.fn_flags & ZEND_ACC_STATIC;
	EG(scope)=function->common.scope;

	zend_create_closure(
	    *return_value,
	    function, function->common.scope, function->common.scope, 
	    this_ptr ? this_ptr : NULL);
	{
		closure = (zend_function *)zend_get_closure_method_def(*return_value);
		if (staticify) {
			closure->common.fn_flags |= ZEND_ACC_STATIC;
		} else closure->common.fn_flags &= ~ZEND_ACC_STATIC;
	}
	EG(scope)=scope;
} /* }}} */

/* {{{ proto Closure uopz_copy(string class, string function)
	   proto Closure uopz_copy(string function) */
PHP_FUNCTION(uopz_copy) {
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("CS", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;

		case 1: if (uopz_parse_parameters("S", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
			return;
	}

	uopz_copy(clazz, name, &return_value, getThis());
} /* }}} */

/* {{{ */
static inline zend_bool uopz_delete(zend_class_entry *clazz, zend_string *name) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	uopz_magic_t *magic = umagic;
	zend_string *lower = zend_string_tolower(name);
	
	if (!table || !zend_hash_exists(table, lower)) {
		if (clazz) {
			uopz_exception(
				"failed to delete the function %s::%s, no overload", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		} else {
			uopz_exception(
				"failed to delete the function %s, no overload", ZSTR_VAL(name));
		}
		zend_string_release(lower);
		return 0;
	}

	uopz_backup(clazz, lower);

	if (!uopz_replace_function(table, lower, NULL, 0)) {
		if (clazz) {
			uopz_exception(
				"failed to delete the function %s::%s, delete failed", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		} else {
			uopz_exception(
				"failed to delete the function %s, delete failed", ZSTR_VAL(name));
		}
		zend_string_release(lower);
		return 0;
	}

	if (clazz) {
		while (magic && magic->name) {
			if (ZSTR_LEN(lower) == magic->length &&
				strncasecmp(ZSTR_VAL(lower), magic->name, magic->length) == SUCCESS) {

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
					case 12: clazz->__debugInfo = NULL; break;
				}
				break;
			}
			magic++;
		}
	}

	zend_string_release(lower);

	return 1;
} /* }}} */

/* {{{ proto bool uopz_delete(mixed function)
	   proto bool uopz_delete(string class, string function) */
PHP_FUNCTION(uopz_delete) {
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("CS", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;

		case 1: if (uopz_parse_parameters("S", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
			return;
	}

	RETVAL_BOOL(uopz_delete(clazz, name));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_restore(zend_class_entry *clazz, zend_string *name) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table),
			  *backups;
	zend_bool result = 0;
	
	backups = zend_hash_index_find_ptr(&UOPZ(backup), (zend_long) table);

	if (backups) {
		zend_string *lower = zend_string_tolower(name);
		uopz_backup_t *backup = zend_hash_find_ptr(backups, lower);
		
		if (backup) {
			result = uopz_replace_function(
					backup->table,
					backup->name,
					backup->function, 0);
		}
		
		zend_string_release(lower);
	}

	return result;
} /* }}} */

/* {{{ proto bool uopz_restore(mixed function)
	   proto bool uopz_restore(string class, string function) */
PHP_FUNCTION(uopz_restore) {
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: if (uopz_parse_parameters("CS", &clazz, &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function)");
			return;
		} break;

		case 1: if (uopz_parse_parameters("S", &name) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (function)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, function) or (function)");
			return;
	}

	RETVAL_BOOL(uopz_restore(clazz, name));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_redefine(zend_class_entry *clazz, zend_string *name, zval *variable) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);

	switch (Z_TYPE_P(variable)) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_STRING:
		case IS_TRUE:
		case IS_FALSE:
		case IS_RESOURCE:
		case IS_NULL:
			break;

		default:
			if (clazz) {
				uopz_exception(
					"failed to redefine the constant %s::%s, type not allowed", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
			} else {
				uopz_exception(
					"failed to redefine the constant %s, type not allowed", ZSTR_VAL(name));
			}
			return 0;
	}

	if (!(zconstant = zend_hash_find_ptr(table, name))) {
		if (!clazz) {
			zend_constant create;

			ZVAL_COPY(&create.value, variable);
			create.flags = CONST_CS;
			create.name = zend_string_copy(name);
			create.module_number = PHP_USER_CONSTANT;

			if (zend_register_constant(&create) != SUCCESS) {
				uopz_exception(
					"failed to redefine the constant %s, operation failed", ZSTR_VAL(name));
				zval_dtor(&create.value);
				return 0;
			}
		} else {
			if (zend_declare_class_constant(clazz, ZSTR_VAL(name), ZSTR_LEN(name), variable) == FAILURE) {
				uopz_exception(
					"failed to redefine the constant %s::%s, update failed", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
				return 0;
			}
			Z_TRY_ADDREF_P(variable);
		}

		return 1;
	}

	if (!clazz) {
		if (zconstant->module_number == PHP_USER_CONSTANT) {
			zval_dtor(&zconstant->value);
			ZVAL_COPY(&zconstant->value, variable);
		} else {
			uopz_exception(
				"failed to redefine the internal %s, not allowed", ZSTR_VAL(name));
			return 0;
		}
	} else {
		zend_hash_del(table, name);
		
		if (zend_declare_class_constant(clazz, ZSTR_VAL(name), ZSTR_LEN(name), variable) == FAILURE) {
			uopz_exception(
				"failed to redefine the constant %s::%s, update failed", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
			return 0;
		}
		Z_TRY_ADDREF_P(variable);
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_redefine(string constant, mixed variable)
	   proto bool uopz_redefine(string class, string constant, mixed variable) */
PHP_FUNCTION(uopz_redefine)
{
	zend_string *name = NULL;
	zval *variable = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 3: {
			if (uopz_parse_parameters("CSz", &clazz, &name, &variable) != SUCCESS) {
				uopz_refuse_parameters(
					"unexpected parameter combination, expected (class, constant, variable)");
				return;
			}
		} break;

		case 2: if (uopz_parse_parameters("Sz", &name, &variable) != SUCCESS) {
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

	if (uopz_redefine(clazz, name, variable)) {
		if (clazz) {
			while ((clazz = clazz->parent)) {
				uopz_redefine(
					clazz, name, variable);
			}
		}
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
} /* }}} */

/* {{{ */
static inline zend_bool uopz_undefine(zend_class_entry *clazz, zend_string *name) {
	zend_constant *zconstant;
	HashTable *table = clazz ? &clazz->constants_table : EG(zend_constants);

	if (!(zconstant = zend_hash_find_ptr(table, name))) {
		return 0;
	}

	if (!clazz) {
		if (zconstant->module_number != PHP_USER_CONSTANT) {
			uopz_exception(
				"failed to undefine the internal constant %s, not allowed", ZSTR_VAL(name));
			return 0;
		}

		if (zend_hash_del(table, name) != SUCCESS) {
			uopz_exception(
				"failed to undefine the constant %s, delete failed", ZSTR_VAL(name));
			return 0;
		}

		return 1;
	}

	if (zend_hash_del(table, name) != SUCCESS) {
		uopz_exception(
			"failed to undefine the constant %s::%s, delete failed", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		return 0;
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_undefine(string constant)
	   proto bool uopz_undefine(string class, string constant) */
PHP_FUNCTION(uopz_undefine)
{
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 2: {
			if (uopz_parse_parameters("CS", &clazz, &name) != SUCCESS) {
				uopz_refuse_parameters(
					"unexpected parameter combination, expected (class, constant)");
				return;
			}
		} break;

		case 1: {
			if (uopz_parse_parameters("S", &name) != SUCCESS) {
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

	if (uopz_undefine(clazz, name)) {
		if (clazz) {
			while ((clazz = clazz->parent)) {
				uopz_undefine(clazz, name);
			}
		}
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
} /* }}} */

/* {{{ */
static zend_bool uopz_function(zend_class_entry *clazz, zend_string *name, zval *closure, zend_long flags, zend_bool ancestry) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_function *destination = NULL;
	zend_function *function =  (zend_function*) zend_get_closure_method_def(closure);
	zend_string *lower = zend_string_tolower(name);

	if (!flags) {
		/* get flags from original function */
		if (uopz_find_function(table, lower, &destination) == SUCCESS) {
			flags = 
				destination->common.fn_flags;
		} else {
			/* set flags to sensible default */
			flags = ZEND_ACC_PUBLIC;
		}
	}

	uopz_backup(clazz, lower);

	function = uopz_copy_function(clazz, function);

	if (!uopz_replace_function(table, lower, function, 0)) {
		destroy_zend_function(function);
		zend_arena_release(&CG(arena), function);
		zend_string_release(lower);

		if (clazz) {
			uopz_exception("failed to create function %s::%s, update failed", ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		} else {
			uopz_exception("failed to create function %s, update failed", ZSTR_VAL(name));
		}

		return 0;
	}

	function->common.fn_flags |= flags & ZEND_ACC_PPP_MASK;

	if (flags & ZEND_ACC_STATIC) {
		function->common.fn_flags |= ZEND_ACC_STATIC;
	}

	if (flags & ZEND_ACC_ABSTRACT) {
		function->common.fn_flags |= ZEND_ACC_ABSTRACT;
	}

	if (clazz) {
		uopz_magic_t *magic = umagic;		

		while (magic && magic->name) {
			if (ZSTR_LEN(name) == magic->length &&
				strncasecmp(ZSTR_VAL(name), magic->name, magic->length) == SUCCESS) {

				switch (magic->id) {
					case 0: clazz->constructor = function; break;
					case 1: clazz->destructor = function; break;
					case 2: clazz->clone = function; break;
					case 3: clazz->__get = function; break;
					case 4: clazz->__set = function; break;
					case 5: clazz->__unset = function; break;
					case 6: clazz->__isset = function; break;
					case 7: clazz->__call = function; break;
					case 8: clazz->__callstatic = function; break;
					case 9: clazz->__tostring = function; break;
					case 10: clazz->serialize_func = function; break;
					case 11: clazz->unserialize_func = function; break;
					case 12: clazz->__debugInfo = function; break;
				}
			}
			magic++;
		}
		function->common.scope = clazz;
	} else {
		function->common.scope = NULL;
	}

	if (clazz && ancestry) {
		zend_class_entry *ce;
		ZEND_HASH_FOREACH_PTR(EG(class_table), ce) {
			if (ce->parent == clazz) {
				uopz_function(ce, name, closure, flags, ancestry);
			}
		} ZEND_HASH_FOREACH_END();
	}

	zend_string_release(lower);

	return 1;
} /* }}} */

/* {{{ proto bool uopz_function(string function, Closure handler [, int flags = 0])
	   proto bool uopz_function(string class, string method, Closure handler [, int flags = 0 [, bool ancestors = true]]) */
PHP_FUNCTION(uopz_function) {
	zend_string *name = NULL;
	zval *closure = NULL;
	zend_class_entry *clazz = NULL;
	zend_long flags = 0;
	zend_bool ancestors = 1;
	
	if (uopz_parse_parameters("SO|l", &name, &closure, zend_ce_closure, &flags) != SUCCESS &&
		uopz_parse_parameters("CSO|lb", &clazz, &name, &closure, zend_ce_closure, &flags, &ancestors) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, "
			"expected "
			"(class, name, closure [, flags [, ancestors]]) or (name, closure [, flags])");
		return;
	}

	RETVAL_BOOL(uopz_function(clazz, name, closure, flags, ancestors));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_implement(zend_class_entry *clazz, zend_class_entry *interface) {
	zend_bool is_final =
		(clazz->ce_flags & ZEND_ACC_FINAL);

	if (!(interface->ce_flags & ZEND_ACC_INTERFACE)) {
		uopz_exception(
			"the class provided (%s) is not an interface", interface->name);
		return 0;
	}

	if (instanceof_function(clazz, interface)) {
		uopz_exception(
			"the class provided (%s) already has the interface interface", clazz->name);
		return 0;
	}

	clazz->ce_flags &= ~ZEND_ACC_FINAL;

	zend_do_implement_interface
		(clazz, interface);

	if (is_final)
		clazz->ce_flags |= ZEND_ACC_FINAL;

	return instanceof_function(clazz, interface);
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

	RETURN_BOOL(uopz_implement(clazz, interface));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_extend(zend_class_entry *clazz, zend_class_entry *parent) {
	zend_bool is_final = clazz->ce_flags & ZEND_ACC_FINAL;

	clazz->ce_flags &= ~ZEND_ACC_FINAL;

	if ((clazz->ce_flags & ZEND_ACC_INTERFACE) &&
		!(parent->ce_flags & ZEND_ACC_INTERFACE)) {
		uopz_exception(
		    "%s cannot extend %s, because %s is not an interface",
		     ZSTR_VAL(clazz->name), ZSTR_VAL(parent->name), ZSTR_VAL(parent->name));
		return 0;
	}

	if (instanceof_function(clazz, parent)) {
		uopz_exception(
			"class %s already extends %s",
			ZSTR_VAL(clazz->name), ZSTR_VAL(parent->name));
		return 0;
	}

	if (parent->ce_flags & ZEND_ACC_TRAIT) {
		zend_do_implement_trait(clazz, parent);
	} else zend_do_inheritance(clazz, parent);

	if (parent->ce_flags & ZEND_ACC_TRAIT)
		zend_do_bind_traits(clazz);

	if (is_final)
		clazz->ce_flags |= ZEND_ACC_FINAL;

	return instanceof_function(clazz, parent);
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

	RETURN_BOOL(uopz_extend(clazz, parent));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_compose(zend_string *name, HashTable *classes, HashTable *methods, HashTable *properties, zend_long flags) {
	HashPosition position[2];
	zend_class_entry *entry;
	zval *member = NULL;
	zend_ulong idx;
	zend_string *lower, *key;
	
	if ((flags & ZEND_ACC_INTERFACE)) {
		if ((properties && zend_hash_num_elements(properties))) {
			uopz_exception(
				"interfaces can not have properties");
			return 0;
		}
	}

	lower = zend_string_tolower(name);

	if (zend_hash_exists(CG(class_table), lower)) {
		uopz_exception(
			"cannot compose existing class (%s)", ZSTR_VAL(name));
		zend_string_release(lower);
		return 0;
	}

	entry = (zend_class_entry*) zend_arena_alloc(&CG(arena), sizeof(zend_class_entry));
	entry->name = zend_string_copy(name);
	entry->type = ZEND_USER_CLASS;
	
	zend_initialize_class_data(entry, 1);

	entry->ce_flags |= flags;
	
	if (!zend_hash_update_ptr(CG(class_table), lower, entry)) {
		uopz_exception(
			"cannot compose class (%s), update failed", ZSTR_VAL(name));
		zend_string_release(lower);
		return 0;
	}

#define uopz_compose_bail(s, ...) do {\
	uopz_exception(s, ##__VA_ARGS__);\
	zend_hash_del(CG(class_table), lower); \
	zend_string_release(lower); \
	return 0; \
} while(0)
	
	if (methods) {
		ZEND_HASH_FOREACH_KEY_VAL(methods, idx, key, member) {
			switch (Z_TYPE_P(member)) {
				 case IS_ARRAY:
					 if (zend_hash_num_elements(Z_ARRVAL_P(member)) == 1)
						 break;

				 case IS_OBJECT:
					 if (instanceof_function(Z_OBJCE_P(member), zend_ce_closure))
						 break;

				 default:
					 uopz_compose_bail("invalid member found in methods array, expects [modifiers => closure], or closure");
			 }
			
			 if (!key) {
				uopz_compose_bail("invalid key found in methods array, expect string keys to be legal function names");
			 }
			
			if (Z_TYPE_P(member) == IS_ARRAY) {
				zend_string *ignored;
				zval *closure;

				ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(member), flags, closure) {
					if (Z_TYPE_P(closure) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(closure), zend_ce_closure)) {
						uopz_compose_bail(
							"invalid member found in methods array, "
							"expects [int => closure], got [int => other]");
					}

					if (!uopz_function(entry, key, closure, flags, 0)) {
						uopz_compose_bail(
							"failed to add method %s to class %s, "
							"previous exceptions occured", ZSTR_VAL(key), ZSTR_VAL(name));
					}
				} ZEND_HASH_FOREACH_END();
			 } else {
				if (!uopz_function(entry, key, member, ZEND_ACC_PUBLIC, 0)) {
			 		uopz_compose_bail(
				 		"failed to add method %s to class %s, "
				 		"previous exceptions occured", ZSTR_VAL(key), ZSTR_VAL(name));
				}
				zend_string_release(key);
			 }
		} ZEND_HASH_FOREACH_END();
	} 

 	if (properties) {
		ZEND_HASH_FOREACH_KEY_VAL(properties, idx, key, member) {
			if (Z_TYPE_P(member) != IS_LONG || !key) {
				uopz_compose_bail(
					"invalid member found in properties array, expects [string => int]");
				break;
			}

			if (zend_declare_property_null(entry, ZSTR_VAL(key), ZSTR_LEN(key), Z_LVAL_P(member)) != SUCCESS) {
				uopz_compose_bail(
					"failed to declare property %s::$%s, engine failure", ZSTR_VAL(entry->name), ZSTR_VAL(key));
				break;
			}
		} ZEND_HASH_FOREACH_END();
	}

	ZEND_HASH_FOREACH_VAL(classes, member) {
		zend_class_entry *parent;
		
		if (Z_TYPE_P(member) != IS_STRING) {
			continue;
		}

		if ((parent = zend_lookup_class(Z_STR_P(member)))) {

			if (entry->ce_flags & ZEND_ACC_TRAIT) {
				if (parent->ce_flags & ZEND_ACC_INTERFACE) {
					uopz_compose_bail(
						"trait %s can not implement interface %s, not allowed",
						ZSTR_VAL(entry->name), ZSTR_VAL(parent->name));
				}
			}

			if (parent->ce_flags & ZEND_ACC_INTERFACE) {
				if (entry->ce_flags & ZEND_ACC_INTERFACE) {
					if (!entry->parent) {
						zend_do_inheritance(entry, parent);
					} else {
						uopz_compose_bail(
							"interface %s may not extend %s, parent of %s already set to %s",
							ZSTR_VAL(entry->name),
							ZSTR_VAL(parent->name),
							ZSTR_VAL(entry->name),
							ZSTR_VAL(entry->parent->name));
					}
				} else zend_do_implement_interface(entry, parent);
			} else if (parent->ce_flags & ZEND_ACC_TRAIT) {
				zend_do_implement_trait(entry, parent);
			} else {
				if (!entry->parent) {
					zend_do_inheritance(entry, parent);
				} else {
					uopz_compose_bail(
						"class %s may not extend %s, parent of %s already set to %s",
						ZSTR_VAL(entry->name),
						ZSTR_VAL(parent->name),
						ZSTR_VAL(entry->name),
						ZSTR_VAL(entry->parent->name));
				}
			}
		}
	} ZEND_HASH_FOREACH_END();

	zend_do_bind_traits(entry);
	zend_string_release(lower);

	return 1;
} /* }}} */

/* {{{ proto bool uopz_compose(string name, array classes [, array methods [, array properties [, int flags = ZEND_ACC_CLASS]]]) */
PHP_FUNCTION(uopz_compose)
{
	zend_string *name = NULL;
	HashTable *classes = NULL;
	HashTable *methods = NULL;
	HashTable *properties = NULL;
	zend_long flags = 0L;

	if (uopz_parse_parameters("Sh|hhl", &name, &classes, &methods, &properties, &flags) != SUCCESS) {
		uopz_refuse_parameters(
			"unexpected parameter combination, expected (string name, array classes [, array methods [, int flags]])");
		return;
	}

	RETURN_BOOL(uopz_compose(name, classes, methods, properties, flags));
} /* }}} */

/* {{{ */
static inline void uopz_flags(zend_class_entry *clazz, zend_string *name, zend_long flags, zval *return_value) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_function *function = NULL;
	zend_long current = 0;

	if (!name || !ZSTR_LEN(name) || !ZSTR_VAL(name)) {
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

	if (uopz_find_function(table, name, &function) != SUCCESS) {
		if (clazz) {
			uopz_exception(
			"failed to set or get flags of %s::%s, function does not exist",
			ZSTR_VAL(clazz->name), ZSTR_VAL(name));
		} else {
			uopz_exception(
				"failed to set or get flags of %s, function does not exist",
				ZSTR_VAL(name));
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
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;
	zend_long flags = LONG_MAX;
	
	switch (ZEND_NUM_ARGS()) {
		case 3: if (uopz_parse_parameters("CSl", &clazz, &name, &flags) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected "
				"(string class, string function, int flags)");
			return;
		} break;

		case 2: if (uopz_parse_parameters("Sl", &name, &flags) != SUCCESS) {
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

	uopz_flags(clazz, name, flags, return_value);
} /* }}} */

/* {{{ uopz */
ZEND_BEGIN_ARG_INFO(uopz_overload_arginfo, 1)
	ZEND_ARG_INFO(0, opcode)
	ZEND_ARG_INFO(0, callable)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_copy__arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_delete_arginfo, 1)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(uopz_restore_arginfo, 1)
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
ZEND_BEGIN_ARG_INFO(uopz_backup_arginfo, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, function)
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
/* }}} */

/* {{{ uopz_functions[]
 */
static const zend_function_entry uopz_functions[] = {
	PHP_FE(uopz_overload, uopz_overload_arginfo)
	PHP_FE(uopz_copy, uopz_copy__arginfo)
	PHP_FE(uopz_delete, uopz_delete_arginfo)
	PHP_FE(uopz_redefine, uopz_redefine_arginfo)
	PHP_FE(uopz_undefine, uopz_undefine_arginfo)
	PHP_FE(uopz_function, uopz_function_arginfo)
	PHP_FE(uopz_backup, uopz_backup_arginfo)
	PHP_FE(uopz_flags, uopz_flags_arginfo)
	PHP_FE(uopz_implement, uopz_implement_arginfo)
	PHP_FE(uopz_extend, uopz_extend_arginfo)
	PHP_FE(uopz_compose, uopz_compose_arginfo)
	PHP_FE(uopz_restore, uopz_restore_arginfo)
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

#ifdef COMPILE_DL_UOPZ
ZEND_GET_MODULE(uopz)
#ifdef ZTS
	ZEND_TSRMLS_CACHE_DEFINE();
#endif
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
