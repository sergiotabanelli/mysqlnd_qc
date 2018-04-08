/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

/* $Id: $ */

#include "php.h"
#include "php_ini.h"
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_classes.h"
#include "mysqlnd_qc_std_handler.h"
/* For offsetof */
#include "stddef.h"

/* {{{ proto object mysqlnd_qc_handler_construct() */
PHP_FUNCTION(mysqlnd_qc_handler_construct)
{
	/* do something here */
}
/* }}} */


HashTable mysqlnd_qc_classes;
static zend_object_handlers mysqlnd_qc_handler_object_handlers;
zend_class_entry * mysqlnd_qc_handler_base_class_entry;
#if 0
static HashTable mysqlnd_qc_handler_properties;
#endif

#ifdef MYSQLND_QC_ENABLED_WHEN_NEEDED_TO_USE

/* {{{ mysqlnd_qc_handler_read_property */
static zval *
#if PHP_VERSION_ID < 50399
mysqlnd_qc_handler_read_property(zval *object, zval *member, int type TSRMLS_DC)
#else
mysqlnd_qc_handler_read_property(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC)
#endif
{
	zval tmp_member;
	zval *retval;
	mysqlnd_qc_handler_object *obj;
	mysqlnd_qc_handler_prop_handler *hnd;
	zend_object_handlers *std_hnd;
	int ret;
	DBG_ENTER("mysqlnd_qc_handler_read_property");

	ret = FAILURE;
	obj = (mysqlnd_qc_handler_object *)zend_objects_get_address(object TSRMLS_CC);

	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}
	DBG_INF_FMT("member=%*s", Z_STRLEN_P(member), Z_STRVAL_P(member));

	if (obj->prop_handler != NULL) {
		ret = zend_hash_find(obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &hnd);
	}
	DBG_INF_FMT("found=%d", ret);
	if (ret == SUCCESS) {
		ret = hnd->read_func(obj, &retval TSRMLS_CC);
		if (ret == SUCCESS) {
			/* ensure we're creating a temporary variable */
			Z_SET_REFCOUNT_P(retval, 0);
		} else {
			retval = EG(uninitialized_zval_ptr);
		}
	} else {
		std_hnd = zend_get_std_object_handlers();
#if PHP_VERSION_ID < 50399
		retval = std_hnd->read_property(object, member, type TSRMLS_CC);
#else
		retval = std_hnd->read_property(object, member, type, key TSRMLS_CC);
#endif

	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
	DBG_RETURN(retval);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_write_property */
static void
#if PHP_VERSION_ID < 50399
mysqlnd_qc_handler_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
#else
mysqlnd_qc_handler_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC)
#endif
{
	zval tmp_member;
	mysqlnd_qc_handler_object *obj;
	mysqlnd_qc_handler_prop_handler *hnd;
	zend_object_handlers *std_hnd;
	int ret;
	DBG_ENTER("mysqlnd_qc_handler_write_property");

	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	ret = FAILURE;
	obj = (mysqlnd_qc_handler_object *)zend_objects_get_address(object TSRMLS_CC);

	if (obj->prop_handler != NULL) {
		ret = zend_hash_find((HashTable *)obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &hnd);
	}
	if (ret == SUCCESS) {
		hnd->write_func(obj, value TSRMLS_CC);
		if (! PZVAL_IS_REF(value) && Z_REFCOUNT_P(value) == 0) {
			Z_ADDREF_P(value);
			zval_ptr_dtor(&value);
		}
	} else {
		std_hnd = zend_get_std_object_handlers();
#if PHP_VERSION_ID < 50399
		std_hnd->write_property(object, member, value TSRMLS_CC);
#else
		std_hnd->write_property(object, member, value, key TSRMLS_CC);
#endif
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_object_has_property */
static int
#if PHP_VERSION_ID < 50399
mysqlnd_qc_handler_object_has_property(zval *object, zval *member, int has_set_exists TSRMLS_DC) /* {{{ */
#else
mysqlnd_qc_handler_object_has_property(zval *object, zval *member, int has_set_exists, const zend_literal *key  TSRMLS_DC) /* {{{ */
#endif
{
	mysqlnd_qc_handler_object *obj = (mysqlnd_qc_handler_object *)zend_objects_get_address(object TSRMLS_CC);
	mysqlnd_qc_handler_prop_handler	p;
	int ret = 0;
	DBG_ENTER("mysqlnd_qc_handler_object_has_property");

	if (zend_hash_find(obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void **)&p) == SUCCESS) {
		switch (has_set_exists) {
			case 2:
				ret = 1;
				break;
			case 1: {
#if PHP_VERSION_ID < 50399
				zval *value = mysqlnd_qc_handler_read_property(object, member, BP_VAR_IS TSRMLS_CC);
#else
				zval *value = mysqlnd_qc_handler_read_property(object, member, BP_VAR_IS, key TSRMLS_CC);
#endif
				if (value != EG(uninitialized_zval_ptr)) {
					convert_to_boolean(value);
					ret = Z_BVAL_P(value)? 1:0;
					/* refcount is 0 */
					Z_ADDREF_P(value);
					zval_ptr_dtor(&value);
				}
				break;
			}
			case 0:{
#if PHP_VERSION_ID < 50399
				zval *value = mysqlnd_qc_handler_read_property(object, member, BP_VAR_IS TSRMLS_CC);
#else
				zval *value = mysqlnd_qc_handler_read_property(object, member, BP_VAR_IS, key TSRMLS_CC);
#endif
				if (value != EG(uninitialized_zval_ptr)) {
					ret = Z_TYPE_P(value) != IS_NULL? 1:0;
					/* refcount is 0 */
					Z_ADDREF_P(value);
					zval_ptr_dtor(&value);
				}
				break;
			}
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid value for has_set_exists");
		}
	}
	DBG_RETURN(ret);
}
/* }}} */

/* {{{ mysqlnd_qc_handler_object_get_debug_info */
static HashTable *
mysqlnd_qc_handler_object_get_debug_info(zval *object, int *is_temp TSRMLS_DC)
{
	mysqlnd_qc_handler_object *obj = (mysqlnd_qc_handler_object *)zend_objects_get_address(object TSRMLS_CC);
	HashTable *retval, *props = obj->prop_handler;
	HashPosition pos;
	mysqlnd_qc_handler_prop_handler *entry;
	DBG_ENTER("mysqlnd_qc_handler_object_get_debug_info");

	ALLOC_HASHTABLE(retval);
	ZEND_INIT_SYMTABLE_EX(retval, zend_hash_num_elements(props) + 1, 0);

	zend_hash_internal_pointer_reset_ex(props, &pos);
	while (zend_hash_get_current_data_ex(props, (void **)&entry, &pos) == SUCCESS) {
		zval member;
		zval *value;
		INIT_ZVAL(member);
		ZVAL_STRINGL(&member, entry->name, entry->name_len, 0);
#if PHP_VERSION_ID < 50399
		value = mysqlnd_qc_handler_read_property(object, &member, BP_VAR_IS TSRMLS_CC);
#else
		value = mysqlnd_qc_handler_read_property(object, &member, BP_VAR_IS, 0 TSRMLS_CC);
#endif
		if (value != EG(uninitialized_zval_ptr)) {
			Z_ADDREF_P(value);
			zend_hash_add(retval, entry->name, entry->name_len + 1, &value, sizeof(zval *), NULL);
		}
		zend_hash_move_forward_ex(props, &pos);
	}

	*is_temp = 1;
	DBG_RETURN(retval);
}
/* }}} */

#endif /* MYSQLND_QC_ENABLED_WHEN_NEEDED_TO_USE */

/* {{{ mysqlnd_qc_handler_read_na */
static int
mysqlnd_qc_handler_read_na(mysqlnd_qc_handler_object *obj, zval **retval TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_handler_read_na");
	*retval = NULL;
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot read property");
	DBG_RETURN(FAILURE);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_write_na */
static int
mysqlnd_qc_handler_write_na(mysqlnd_qc_handler_object *obj, zval *newval TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_handler_write_na");
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot write property");
	DBG_RETURN(FAILURE);
}
/* }}} */

/* {{{ mysqlnd_qc_handler_add_property */
void
mysqlnd_qc_handler_add_property(HashTable *h, const char *pname, size_t pname_len, mysqlnd_qc_handler_read_t r_func, mysqlnd_qc_handler_write_t w_func TSRMLS_DC)
{
	mysqlnd_qc_handler_prop_handler p;

	p.name = (char*) pname;
	p.name_len = pname_len;
	p.read_func = (r_func) ? r_func : mysqlnd_qc_handler_read_na;
	p.write_func = (w_func) ? w_func : mysqlnd_qc_handler_write_na;
	zend_hash_add(h, pname, pname_len + 1, &p, sizeof(mysqlnd_qc_handler_prop_handler), NULL);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_objects_free_storage */
static void
mysqlnd_qc_handler_objects_free_storage(void *object TSRMLS_DC)
{
	zend_object *zo = (zend_object *)object;
	mysqlnd_qc_handler_object 	*intern = (mysqlnd_qc_handler_object *)zo;

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_free_storage */
static void
mysqlnd_qc_handler_free_storage(void *object TSRMLS_DC)
{
	mysqlnd_qc_handler_object *intern = (mysqlnd_qc_handler_object *)object;
	DBG_ENTER("mysqlnd_qc_handler_free_storage");
	if (intern->ptr) {
		if (intern->zo.ce == mysqlnd_qc_handler_default_class_entry) {}
	}
	mysqlnd_qc_handler_objects_free_storage(object TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ php_mysqlnd_qc_handler_constructor_get() */
static union _zend_function *
php_mysqlnd_qc_handler_constructor_get(zval *object TSRMLS_DC)
{
	zend_class_entry * ce = Z_OBJCE_P(object);
	DBG_ENTER("php_mysqlnd_qc_handler_constructor_get");

	if (ce != mysqlnd_qc_handler_base_class_entry && ce != mysqlnd_qc_handler_default_class_entry) {
		DBG_INF("not our class");
		DBG_RETURN(zend_std_get_constructor(object TSRMLS_CC));
	} else {
		static zend_internal_function f;
		mysqlnd_qc_handler_object *obj = (mysqlnd_qc_handler_object *)zend_objects_get_address(object TSRMLS_CC);

		f.function_name = obj->zo.ce->name;
		f.scope = obj->zo.ce;
		f.arg_info = NULL;
		f.num_args = 0;
		f.fn_flags = 0;

		f.type = ZEND_INTERNAL_FUNCTION;
		if (obj->zo.ce == mysqlnd_qc_handler_base_class_entry) {
			DBG_INF("our base class");
			f.handler = PHP_FN(mysqlnd_qc_handler_construct);
		} else if (obj->zo.ce == mysqlnd_qc_handler_default_class_entry) {
			DBG_INF("mysqlnd_qc_handler_default_class_entry");
			f.handler = PHP_FN(mysqlnd_qc_handler_default_construct);
		}

		DBG_RETURN( (union _zend_function*)&f );
	}
}
/* }}} */


/* {{{ mysqlnd_qc_handler_objects_new */
PHP_MYSQLND_QC_EXPORT(zend_object_value) mysqlnd_qc_handler_objects_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	mysqlnd_qc_handler_object *intern;
#if PHP_VERSION_ID < 50399
	zval *tmp;
#endif
	zend_class_entry *mysqlnd_qc_handler_base_class;
	zend_objects_free_object_storage_t free_storage;
	DBG_ENTER("mysqlnd_qc_handler_objects_new");

	intern = emalloc(sizeof(mysqlnd_qc_handler_object));
	memset(intern, 0, sizeof(mysqlnd_qc_handler_object));
	intern->ptr = NULL;
	intern->prop_handler = NULL;

	mysqlnd_qc_handler_base_class = class_type;
	while (mysqlnd_qc_handler_base_class->type != ZEND_INTERNAL_CLASS &&
		   mysqlnd_qc_handler_base_class->parent != NULL) {
		mysqlnd_qc_handler_base_class = mysqlnd_qc_handler_base_class->parent;
	}
	zend_hash_find(&mysqlnd_qc_classes, mysqlnd_qc_handler_base_class->name, mysqlnd_qc_handler_base_class->name_length + 1, (void **) &intern->prop_handler);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#else
	object_properties_init(&intern->zo, class_type);
#endif

	/* Base class should be last */
	if (instanceof_function(class_type, mysqlnd_qc_handler_default_class_entry TSRMLS_CC)) {
		free_storage = mysqlnd_qc_handler_free_storage;
	} else if (instanceof_function(class_type, mysqlnd_qc_handler_base_class_entry TSRMLS_CC)) {
		free_storage = mysqlnd_qc_handler_free_storage;
	}

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, free_storage, NULL TSRMLS_CC);
	retval.handlers = &mysqlnd_qc_handler_object_handlers;

	DBG_RETURN(retval);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_base_methods[] */
const zend_function_entry mysqlnd_qc_handler_base_methods[] = {
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_GET_HASH_KEY_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_FIND_IN_CACHE_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_RETURN_TO_CACHE_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_ADD_TO_CACHE_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_UPDATE_CACHE_STATS_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_FILL_STATS_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_CLEAR_CACHE_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_INIT_METHOD, NULL)
	ZEND_ABSTRACT_ME("mysqlnd_qc_handler", MYSQLND_QC_SHUTDOWN_METHOD, NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ mysqlnd_qc_handler_base_minit */
static void
mysqlnd_qc_handler_base_minit(TSRMLS_D)
{
	REGISTER_MYSQLND_QC_INTERFACE("mysqlnd_qc_handler", mysqlnd_qc_handler_base_class_entry, mysqlnd_qc_handler_base_methods);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_base_mshutdown */
static void
mysqlnd_qc_handler_base_mshutdown(TSRMLS_D)
{
#if 0
	zend_hash_destroy(&mysqlnd_qc_handler_properties);
#endif
}
/* }}} */


/* {{{ mysqlnd_qc_handler_object_minit */
void
mysqlnd_qc_handler_classes_minit(TSRMLS_D)
{
	zend_object_handlers *std_hnd = zend_get_std_object_handlers();

	zend_hash_init(&mysqlnd_qc_classes, 0, NULL, NULL, 1);

	memcpy(&mysqlnd_qc_handler_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	mysqlnd_qc_handler_object_handlers.clone_obj			= NULL;
	/* We define which functions are used thus we actually don't need these directly here */
#ifdef MYSQLND_QC_ENABLED_WHEN_NEEDED_TO_USE
	mysqlnd_qc_handler_object_handlers.read_property		= mysqlnd_qc_handler_read_property;
	mysqlnd_qc_handler_object_handlers.write_property		= mysqlnd_qc_handler_write_property;
	mysqlnd_qc_handler_object_handlers.has_property			= mysqlnd_qc_handler_object_has_property;
	mysqlnd_qc_handler_object_handlers.get_debug_info		= mysqlnd_qc_handler_object_get_debug_info;
#endif

	mysqlnd_qc_handler_object_handlers.get_property_ptr_ptr = std_hnd->get_property_ptr_ptr;
	mysqlnd_qc_handler_object_handlers.get_constructor		= php_mysqlnd_qc_handler_constructor_get;

	mysqlnd_qc_handler_base_minit(TSRMLS_C);
}


/* {{{ mysqlnd_qc_handler_object_mshutdown */
void
mysqlnd_qc_handler_classes_mshutdown(TSRMLS_D)
{
	mysqlnd_qc_handler_base_mshutdown(TSRMLS_C);

	zend_hash_destroy(&mysqlnd_qc_classes);
}
/* }}} */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
