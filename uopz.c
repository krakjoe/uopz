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
#include "Zend/zend_extensions.h"
#include "Zend/zend_string.h"
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
#include "compile.h"
#include "copy.h"

ZEND_DECLARE_MODULE_GLOBALS(uopz)

#define MAX_OPCODE 163
#undef EX
#define EX(element) EG(current_execute_data)->element
#define OPLINE EX(opline)
#define OPCODE OPLINE->opcode

# define EX_T(offset) (*EX_TMP_VAR(EG(current_execute_data), offset))

#define AI_SET_PTR(t, val) do {				\
	temp_variable *__t = (t);			\
	__t->var.ptr = (val);				\
	__t->var.ptr_ptr = &__t->var.ptr;		\
} while (0)

#define RETURN_VALUE_USED(opline) (!((opline)->result_type & EXT_TYPE_UNUSED))
#define ZEND_VM_CONTINUE()         return 0

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

static zend_bool uopz_backup(zend_class_entry *scope, zend_string *key);

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

/* {{{ */
typedef struct _uopz_backup_t {
	zend_string      *name;
	zend_class_entry *scope;
	zend_function    *internal;
} uopz_backup_t; /* }}} */

/* {{{ this is awkward, but finds private functions ... so don't "fix" it ... */
static int uopz_find_function(HashTable *table, zend_string *name, zend_function **function) {
	Bucket *bucket;
	zend_string *lower  = zend_string_tolower(name);

	ZEND_HASH_FOREACH_BUCKET(table, bucket) {
		if ((ZSTR_LEN(bucket->key) == ZSTR_LEN(lower)) && 
			(bucket->h == ZSTR_HASH(lower)) &&
			memcmp(ZSTR_VAL(lower), ZSTR_VAL(bucket->key), ZSTR_LEN(lower)) == SUCCESS) {
			if (function) {
				*function = (zend_function*) Z_PTR(bucket->val);
			}
			zend_string_release(lower);
			return SUCCESS;
		}
	} ZEND_HASH_FOREACH_END();

	zend_string_release(lower);

	return FAILURE;
} /* }}} */

/* {{{ */
static void php_uopz_init_globals(zend_uopz_globals *ng) {
	ng->ini.backup = 1;
	ng->ini.overloads = 0;
	ng->ini.fixup = 0;
} /* }}} */

/* {{{ */
static void php_uopz_backup_dtor(zval *el) {
	zval container;
	uopz_backup_t *backup = (uopz_backup_t *) Z_PTR_P(el);
	HashTable *table = backup->scope ?
		&backup->scope->function_table :
		CG(function_table);

	if (backup->internal->type == ZEND_INTERNAL_FUNCTION) {
		zend_hash_update_mem(
			table, backup->name, backup->internal, sizeof(zend_internal_function));
	} else {
		zend_hash_del(table, backup->name);
		destroy_zend_function(backup->internal);
	}
	if (backup->scope) {
		backup->scope->refcount--;
	}
	
	zend_string_release(backup->name);
	efree(backup);
} /* }}} */

/* {{{ */
static void php_uopz_backup_table_dtor(zval *zv) {
	zend_hash_destroy((HashTable*) Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
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

				fci.params = (zval*) emalloc(2 * sizeof(zval));
				ZVAL_UNDEF(&fci.params[0]);
				ZVAL_UNDEF(&fci.params[1]);
				fci.param_count = 2;
				fci.no_separation = 0;

				switch (OPCODE) {
					case ZEND_INSTANCEOF: {
						GET_OP1(BP_VAR_R);

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
						ZVAL_STR(&fci.params[1], oce->name);
					} break;

					case ZEND_ADD_INTERFACE:
					case ZEND_ADD_TRAIT: {
						oce = Z_CE_P(EX_VAR(OPLINE->op1.var));
						
						if (oce) {
							ZVAL_STR(&fci.params[0], oce->name);
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

						ZVAL_STR(&fci.params[1], oce->name);
					} break;

					case ZEND_NEW: {
						if (OPLINE->op1_type == IS_CONST) {
							ZVAL_COPY(&fci.params[0], EX_CONSTANT(OPLINE->op1));
						} else {
							oce = Z_CE_P(EX_VAR(OPLINE->op1.var));
							ZVAL_STR(&fci.params[0], oce->name);
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
								ZVAL_STR(&fci.params[0], oce->name);
							} else if (Z_TYPE_P(name) == IS_STRING) {
								oce = zend_fetch_class_by_name(Z_STR_P(name), name+1, 
									ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
								if (!oce) {
									ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
								}
								ZVAL_STR(&fci.params[0], oce->name);
							} else {
								if (EG(exception)) {
									HANDLE_EXCEPTION();
								}
							}
						}
					} break;

					case ZEND_EXIT: {
						GET_OP1(BP_VAR_RW);
						fci.param_count = 1;
					} break;

					default: {
						GET_OP1(BP_VAR_RW);
						GET_OP2(BP_VAR_RW);
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

				switch (OPCODE) {
					case ZEND_INSTANCEOF: {
						convert_to_string(&fci.params[1]);

						nce = zend_lookup_class(Z_STR(fci.params[1]));

						if (nce != oce) {
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
							CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op1)), nce);
						}
					} break;

					case ZEND_FETCH_CLASS: {
						convert_to_string(&fci.params[0]);
						
						nce = zend_lookup_class(Z_STR(fci.params[0]));
	
						if (nce != oce) {
							CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(OPLINE->op2)), nce);
						}
					} break;

					case ZEND_EXIT: {
						if (dispatching == ZEND_USER_OPCODE_CONTINUE) {
							if (EX(opline) < &EX(func)->op_array.opcodes[EX(func)->op_array.last - 1]) {
								ZEND_VM_JMP(OPLINE + 1);
								if (Z_TYPE(fci.params[0]) != IS_UNDEF)
									zval_ptr_dtor(&fci.params[0]);
								if (Z_TYPE(fci.params[1]) != IS_UNDEF)
									zval_ptr_dtor(&fci.params[1]);
				
								if (fci.params)
									efree(fci.params);

								return ZEND_USER_OPCODE_CONTINUE;
							}
						}

						if (Z_TYPE(fci.params[0]) != IS_UNDEF)
							zval_ptr_dtor(&fci.params[0]);
						if (Z_TYPE(fci.params[1]) != IS_UNDEF)
							zval_ptr_dtor(&fci.params[1]);
				
						if (fci.params)
							efree(fci.params);

						return ZEND_USER_OPCODE_RETURN;
					} break;
				}

				if (Z_TYPE(fci.params[0]) != IS_UNDEF)
					zval_ptr_dtor(&fci.params[0]);
				if (Z_TYPE(fci.params[1]) != IS_UNDEF)
					zval_ptr_dtor(&fci.params[1]);
				
				if (fci.params)
					efree(fci.params);
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
static inline void php_uopz_backup() {
	zend_class_entry *scope;	
	zend_string *name;
	zend_ulong idx;

	ZEND_HASH_FOREACH_KEY(CG(function_table), idx, name) {
		uopz_backup(NULL, name);
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_FOREACH_PTR(CG(class_table), scope) {
		if (!(scope->ce_flags & ZEND_ACC_INTERFACE)) {
			ZEND_HASH_FOREACH_KEY(&scope->function_table, idx, name) {
				uopz_backup(scope, name);
			} ZEND_HASH_FOREACH_END();		
		}
	} ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ */
static inline void php_uopz_init_handlers(int module) {
	memset(ohandlers, 0, sizeof(user_opcode_handler_t) * MAX_OPCODE);
	
#define REGISTER_ZEND_UOPCODE(u) \
	zend_register_long_constant\
		((u)->name, (u)->length, (u)->code, CONST_CS|CONST_PERSISTENT, module)

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

#undef REGISTER_ZEND_UOPCODE
} /* }}} */

static int uopz_zend_startup(zend_extension *extension) /* {{{ */
{
	return zend_startup_module(&uopz_module_entry);
}
/* }}} */

#ifndef ZEND_EXT_API
#define ZEND_EXT_API    ZEND_DLEXPORT
#endif
ZEND_EXTENSION();

static inline void php_uopz_overload_exit(zend_op_array *op_array);

ZEND_EXT_API zend_extension zend_extension_entry = {
	PHP_UOPZ_EXTNAME,
	PHP_UOPZ_VERSION,
	"Joe Watkins <krakjoe@php.net>",
	"https://github.com/krakjoe/uopz",
	"Copyright (c) 2014",
	uopz_zend_startup,
	NULL,           	/* shutdown_func_t */
	NULL,                   /* activate_func_t */
	NULL,                   /* deactivate_func_t */
	NULL,           	/* message_handler_func_t */
	NULL,	 		/* op_array_handler_func_t */
	NULL, 			/* statement_handler_func_t */
	NULL,             	/* fcall_begin_handler_func_t */
	NULL,           	/* fcall_end_handler_func_t */
	NULL,      		/* op_array_ctor_func_t */
	NULL,      		/* op_array_dtor_func_t */
	STANDARD_ZEND_EXTENSION_PROPERTIES
};

typedef void (*zend_execute_ex_t) (zend_execute_data *);
typedef void (*zend_execute_internal_t) (zend_execute_data *, zval *);

zend_execute_ex_t zend_execute_ex_function = NULL;
zend_execute_internal_t zend_execute_internal_function = NULL;

static inline void uopz_execute_ex(zend_execute_data *ex) {
	if (zend_execute_ex_function) {
		zend_execute_ex_function(ex);
	} else execute_ex(ex);
}

static inline void uopz_execute_internal(zend_execute_data *ex, zval *retval) {
	if (zend_execute_internal_function) {
		zend_execute_internal_function(ex, retval);
	} else execute_internal(ex, retval);
}

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(uopz)
{
	if (!zend_get_extension("uopz")) {
		zend_extension_entry.startup = NULL;
		zend_register_extension(&zend_extension_entry, NULL);
	}

	ZEND_INIT_MODULE_GLOBALS(uopz, php_uopz_init_globals, NULL);

	UOPZ(copts) = CG(compiler_options);
	
	/* do not generate INIT_FCALL as these have a fixed vm_stack frame size */
	CG(compiler_options) |= ZEND_COMPILE_HANDLE_OP_ARRAY | ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION | ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS | ZEND_COMPILE_IGNORE_USER_FUNCTIONS | ZEND_COMPILE_GUARDS;

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

	php_uopz_init_handlers(module_number);

	if (UOPZ(ini).fixup) {
		CG(class_table)->pDestructor = NULL;
		CG(function_table)->pDestructor = NULL;
	}
	
	zend_execute_internal_function = zend_execute_internal;
	zend_execute_internal = uopz_execute_internal;
	zend_execute_ex_function = zend_execute_ex;
	zend_execute_ex = uopz_execute_ex;

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
	zend_class_entry *ce = NULL;

	zend_string *spl = zend_string_init(ZEND_STRL("RuntimeException"), 0);
	spl_ce_RuntimeException =
			(ce = zend_lookup_class(spl)) ?
				ce : zend_exception_get_default();
	zend_string_release(spl);
	
	spl = zend_string_init(ZEND_STRL("InvalidArgumentException"), 0);	
	spl_ce_InvalidArgumentException =
			(ce = zend_lookup_class(spl)) ?
				ce : zend_exception_get_default();
	zend_string_release(spl);
	
	zend_hash_init(
		&UOPZ(overload), 8, NULL,
		(dtor_func_t) ZVAL_PTR_DTOR, 0);
	zend_hash_init(
		&UOPZ(backup), 8, NULL,
		(dtor_func_t) php_uopz_backup_table_dtor, 0);

	

	return SUCCESS;
} /* }}} */

/* {{{ */
static inline int php_uopz_clean_user_function(zval *zv) {
	zend_function *function = (zend_function*) Z_PTR_P(zv);

	if (function->type == ZEND_USER_FUNCTION) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ */
static inline int php_uopz_clean_user_class(zval *zv) {
	zend_class_entry *ce = (zend_class_entry*) Z_PTR_P(zv);

	zend_hash_apply(&ce->function_table, php_uopz_clean_user_function);

	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(uopz)
{
	zend_hash_destroy(&UOPZ(overload));
	zend_hash_destroy(&UOPZ(backup));

	zend_hash_apply(CG(function_table), php_uopz_clean_user_function);
	zend_hash_apply(CG(class_table), php_uopz_clean_user_class);

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
static zend_bool uopz_backup(zend_class_entry *clazz, zend_string *name) {
	HashTable     *backups = NULL;
	zend_function *function = NULL;
	HashTable     *table = (clazz) ? &clazz->function_table : CG(function_table);
	
	if (uopz_find_function(table, name, &function) != SUCCESS) {
		return 0;
	}

	if (!(backups = zend_hash_index_find_ptr(&UOPZ(backup), (zend_ulong) table))) {
		HashTable creating;
		zend_hash_init(&creating, 8, NULL, (dtor_func_t) php_uopz_backup_dtor, 0);
		backups = zend_hash_index_add_mem(
			&UOPZ(backup),
			(zend_ulong) table, &creating, sizeof(HashTable));
	}

	if (!zend_hash_exists(backups, name)) {
		uopz_backup_t backup;

		backup.scope = clazz;
		backup.internal = uopz_copy_function(function);

		if (backup.internal->type == ZEND_INTERNAL_FUNCTION) {
			backup.name = zend_string_dup(name, 1);
		} else backup.name = zend_string_copy(name);

		if (!zend_hash_add_mem(
			backups,
			backup.name,
			&backup,
			sizeof(uopz_backup_t))) {
			if (clazz) {
				uopz_exception(
					"backup of %s::%s failed, update failed",
					clazz->name->val, name->val);
			} else {
				uopz_exception(
					"backup of %s failed, update failed",
					name->val);
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

	RETVAL_BOOL(uopz_backup(clazz, name));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_restore(zend_class_entry *clazz, zend_string *name, zend_bool ancestry) {
	zend_function *function = NULL;
	HashTable     *backup = NULL;
	uopz_backup_t *ubackup = NULL;
	HashTable     *table = clazz ? &clazz->function_table : CG(function_table);

	if (!(backup = zend_hash_index_find_ptr(&UOPZ(backup), (zend_ulong) table))) {
		if (clazz) {
			uopz_exception(
				"restoration of %s::%s failed, no backups for the class %s could be found",
				clazz->name->val, name->val, clazz->name->val);
		} else {
			uopz_exception(
				"restoration of %s failed, no backup could be found",
				name->val);
		}
		return 0;
	}

	if (!(ubackup = zend_hash_find_ptr(backup, name))) {
		/* nothing to restore */
		return 1;
	}

	table = ubackup->scope ?
		&ubackup->scope->function_table :
		CG(function_table);

	if (zend_hash_update_ptr(
		table,
		ubackup->name, ubackup->internal)) {
		function_add_ref(ubackup->internal);

		if (clazz && ancestry) {
			zend_class_entry *ce;
			
			ZEND_HASH_FOREACH_PTR(EG(class_table), ce) {
				if (ce->parent == clazz) {
					uopz_restore(ce, name, ancestry);
				}
			} ZEND_HASH_FOREACH_END();
		}
	} else {
		if (clazz) {
			uopz_exception(
				"restoration of %s::%s failed, update failed",
				clazz->name->val, name->val, name->val);
		} else {
			uopz_exception(
				"restoration of %s failed, update failed",
				name->val);
		}
		return 0;
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_restore(string class, string function [, bool ancestry = true])
	   proto bool uopz_restore(string function) */
PHP_FUNCTION(uopz_restore) {
	zend_string *name = NULL;
	zend_class_entry *clazz = NULL;
	zend_bool ancestry = 1;

	switch (ZEND_NUM_ARGS()) {
		case 3:
		case 2: if (uopz_parse_parameters("CS|b", &clazz, &name, &ancestry) != SUCCESS) {
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
				"unexpected parameter combination, expected (class, function [, ancestry]) or (function) expected");
			return;
	}

	RETVAL_BOOL(uopz_restore(clazz, name, ancestry));
} /* }}} */

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
				clazz->name->val, name->val);
		} else {
			uopz_exception("could not find the requested function (%s)", name->val);
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
static inline zend_bool uopz_rename(zend_class_entry *clazz, zend_string *name, zend_string *rename) {
	zend_function *tuple[2] = {NULL, NULL};
	size_t size[2];
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);

	if (!name->val && !rename->val) {
		return 0;
	}

	if (zend_string_equals_ci(name, rename)) {
		return 0;
	}

	uopz_find_function(table, name, &tuple[0]);
	uopz_find_function(table, rename, &tuple[1]);

	if (!tuple[0] && !tuple[1]) {
		if (clazz) {
			uopz_exception(
				"failed to find the functions %s::%s and %s::%s",
				clazz->name->val, name->val, clazz->name->val, rename->val);
		} else {
			uopz_exception(
				"failed to find the functions %s and %s",
				name->val, rename->val);
		}
		return 0;
	}

	if (tuple[0] && tuple[1]) {
		dtor_func_t dtor_backup = table->pDestructor;
		table->pDestructor = NULL; /* do not free the pointers */

		size[0] = tuple[0]->type == ZEND_INTERNAL_FUNCTION ? sizeof(zend_internal_function) : sizeof(zend_op_array);
		size[1] = tuple[1]->type == ZEND_INTERNAL_FUNCTION ? sizeof(zend_internal_function) : sizeof(zend_op_array);

		if ((tuple[1] = zend_hash_update_mem(table, name, tuple[1], size[1])) &&
		    (tuple[0] = zend_hash_update_mem(table, rename, tuple[0], size[0]))) {
			table->pDestructor = dtor_backup;
			return 1;
		}

		if (clazz) {
			uopz_exception(
				"failed to rename the functions %s::%s and %s::%s, switch failed",
				clazz->name->val, name->val, clazz->name->val, rename->val);
		} else {
			uopz_exception(
				"failed to rename the functions %s and %s, switch failed",
				name->val, rename->val);
		}

		table->pDestructor = dtor_backup;
		return 0;

	}

	/* only one existing function */
	if (tuple[1]) {
		tuple[0] = tuple[1];
	}
	size[0] = tuple[0]->type == ZEND_INTERNAL_FUNCTION ? sizeof(zend_internal_function) : sizeof(zend_op_array);
	
	{
		if (!(tuple[0] = zend_hash_update_mem(table, rename, (void**) tuple[0], size[0]))) {
			if (clazz) {
				uopz_exception(
					"failed to rename the function %s::%s to %s::%s, update failed",
					clazz->name->val, name->val, clazz->name->val, rename->val);
			} else {
				uopz_exception(
					"failed to rename the function %s to %s, update failed",
					name->val, rename->val);
			}
			return 0;
		}

		function_add_ref(tuple[0]);
	}

	return 1;
} /* }}} */

/* {{{ proto bool uopz_rename(mixed name, mixed rename)
	   proto bool uopz_rename(string class, mixed name, mixed rename) */
PHP_FUNCTION(uopz_rename) {
	zend_string *name = NULL;
	zend_string *rename = NULL;
	zend_class_entry *clazz = NULL;

	switch (ZEND_NUM_ARGS()) {
		case 3: if (uopz_parse_parameters("CSS", &clazz, &name, &rename) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, name, rename)");
			return;
		} break;

		case 2: if (uopz_parse_parameters("SS", &name, &rename) != SUCCESS) {
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (name, rename)");
			return;
		} break;

		default:
			uopz_refuse_parameters(
				"unexpected parameter combination, expected (class, name, rename) or (name, rename)");
			return;
	}

	RETVAL_BOOL(uopz_rename(clazz, name, rename));
} /* }}} */

/* {{{ */
static inline zend_bool uopz_delete(zend_class_entry *clazz, zend_string *name) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	uopz_magic_t *magic = umagic;

	uopz_backup(clazz, name);

	if (!zend_hash_exists(table, name)) {
		if (clazz) {
			uopz_exception(
				"failed to delete the function %s::%s, it does not exist", clazz->name->val, name->val);
		} else {
			uopz_exception(
				"failed to delete the function %s, it does not exist", name->val);
		}
		return 0;
	}

	if (zend_hash_del(table, name) != SUCCESS) {
		if (clazz) {
			uopz_exception(
				"failed to delete the function %s::%s, delete failed", clazz->name->val, name->val);
		} else {
			uopz_exception(
				"failed to delete the function %s, delete failed", name->val);
		}
		return 0;
	}

	if (clazz) {
		while (magic && magic->name) {
			if (name->len == magic->length &&
				strncasecmp(name->val, magic->name, magic->length) == SUCCESS) {

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
					"failed to redefine the constant %s::%s, type not allowed", clazz->name, name->val);
			} else {
				uopz_exception(
					"failed to redefine the constant %s, type not allowed", name->val);
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
					"failed to redefine the constant %s, operation failed", name->val);
				zval_dtor(&create.value);
				return 0;
			}
		} else {
			if (!zend_hash_update(table, name, variable)) {
				uopz_exception(
					"failed to redefine the constant %s::%s, update failed", clazz->name->val, name->val);
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
				"failed to redefine the internal %s, not allowed", name->val);
			return 0;
		}
	} else {
		if (!zend_hash_update(table, name, variable)) {
			uopz_exception(
				"failed to redefine the constant %s::%s, update failed", clazz->name->val, name->val);
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
				"failed to undefine the internal constant %s, not allowed", name->val);
			return 0;
		}

		if (zend_hash_del(table, name) != SUCCESS) {
			uopz_exception(
				"failed to undefine the constant %s, delete failed", name->val);
			return 0;
		}

		return 1;
	}

	if (zend_hash_del(table, name) != SUCCESS) {
		uopz_exception(
			"failed to undefine the constant %s::%s, delete failed", clazz->name, name->val);
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
static inline zend_bool uopz_function(zend_class_entry *clazz, zend_string *name, zval *closure, zend_long flags, zend_bool ancestry) {
	HashTable *table = clazz ? &clazz->function_table : CG(function_table);
	zend_function *destination = NULL;
	zend_function *function = (zend_function*) zend_get_closure_method_def(closure);
	zend_string *lower = zend_string_tolower(name);

	uopz_backup(clazz, lower);

	if (!flags) {
		/* get flags from original function */
		if (uopz_find_function(table, lower, &destination) == SUCCESS) {
			flags = destination->common.fn_flags;
		} else {
			/* set flags to sensible default */
			flags = ZEND_ACC_PUBLIC;
		}

		destination = NULL;
	}
	
	destination = uopz_copy_function(function);
	
	if (!zend_hash_update_ptr(table, lower, destination)) {
		zend_arena_release(&CG(arena), destination);
		zend_string_release(lower);

		if (clazz) {
			uopz_exception("failed to create function %s::%s, update failed", clazz->name->val, name->val);
		} else {
			uopz_exception("failed to create function %s, update failed", name->val);
		}

		return 0;
	}
	
	destination->common.fn_flags = flags;
	destination->common.prototype = destination;

	if (clazz) {
		uopz_magic_t *magic = umagic;		

		while (magic && magic->name) {
			if (name->len == magic->length &&
				strncasecmp(name->val, magic->name, magic->length) == SUCCESS) {

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
					case 12: clazz->__debugInfo = destination; break;
				}
			}
			magic++;
		}
		destination->common.scope = clazz;
	} else {
		destination->common.scope = NULL;
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
		     clazz->name->val, parent->name->val, parent->name->val);
		return 0;
	}

	if (instanceof_function(clazz, parent)) {
		uopz_exception(
			"class %s already extends %s",
			clazz->name->val, parent->name->val);
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
			"cannot compose existing class (%s)", name->val);
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
			"cannot compose class (%s), update failed", name->val);
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
							"previous exceptions occured", key->val, name->val);
					}
				} ZEND_HASH_FOREACH_END();
			 } else {
				if (!uopz_function(entry, key, member, ZEND_ACC_PUBLIC, 0)) {
			 		uopz_compose_bail(
				 		"failed to add method %s to class %s, "
				 		"previous exceptions occured", key->val, name->val);
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

			if (zend_declare_property_null(entry, key->val, key->len, Z_LVAL_P(member)) != SUCCESS) {
				uopz_compose_bail(
					"failed to declare property %s::$%s, engine failure", entry->name->val, key->val);
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
						entry->name->val, parent->name->val);
				}
			}

			if (parent->ce_flags & ZEND_ACC_INTERFACE) {
				if (entry->ce_flags & ZEND_ACC_INTERFACE) {
					if (!entry->parent) {
						zend_do_inheritance(entry, parent);
					} else {
						uopz_compose_bail(
							"interface %s may not extend %s, parent of %s already set to %s",
							entry->name->val,
							parent->name->val,
							entry->name->val,
							entry->parent->name->val);
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
						entry->name->val,
						parent->name->val,
						entry->name->val,
						entry->parent->name->val);
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
	zend_long flags = 0;

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

	if (!name) {
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
			clazz->name->val, name->val);
		} else {
			uopz_exception(
				"failed to set or get flags of %s, function does not exist",
				name->val);
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
	ZEND_ARG_INFO(0, status)
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
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
