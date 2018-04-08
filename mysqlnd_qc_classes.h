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

#ifndef MYSQLND_QC_CLASSES_H
#define MYSQLND_QC_CLASSES_H


extern zend_class_entry * mysqlnd_qc_handler_class_entry;

typedef struct st_mysqlnd_qc_handler_object {
	zend_object zo;
	void 		*ptr;
	HashTable 	*prop_handler;
} mysqlnd_qc_handler_object; /* extends zend_object */


typedef struct st_mysqlnd_qc_handler_property_entry {
	const char *pname;
	size_t pname_length;
	int (*r_func)(mysqlnd_qc_handler_object *obj, zval **retval TSRMLS_DC);
	int (*w_func)(mysqlnd_qc_handler_object *obj, zval *value TSRMLS_DC);
} mysqlnd_qc_handler_property_entry;


typedef int (*mysqlnd_qc_handler_read_t)(mysqlnd_qc_handler_object *obj, zval **retval TSRMLS_DC);
typedef int (*mysqlnd_qc_handler_write_t)(mysqlnd_qc_handler_object *obj, zval *newval TSRMLS_DC);

typedef struct st_mysqlnd_qc_handler_prop_handler {
	char *name;
	size_t name_len;
	mysqlnd_qc_handler_read_t read_func;
	mysqlnd_qc_handler_write_t write_func;
} mysqlnd_qc_handler_prop_handler;


#define REGISTER_MYSQLND_QC_CLASS(name, class_entry, class_functions, parent_ce, parent_name) { \
	zend_class_entry ce; \
	INIT_CLASS_ENTRY(ce, (name), (class_functions)); \
	ce.create_object = mysqlnd_qc_handler_objects_new; \
	(class_entry) = zend_register_internal_class_ex(&ce, (parent_ce), (parent_name) TSRMLS_CC); \
}


#define REGISTER_MYSQLND_QC_INTERFACE(name, interface_entry, class_functions) { \
	zend_class_entry ce; \
	INIT_CLASS_ENTRY_EX(ce, (name), sizeof((name)) - 1, (class_functions)); \
	(interface_entry) = zend_register_internal_interface(&ce TSRMLS_CC); \
}


#define MYSQLND_QC_ADD_PROPERTIES(a,b) \
{ \
	unsigned int i = 0; \
	while (b[i].pname != NULL) { \
		mysqlnd_qc_handler_add_property((a), (b)[i].pname, (b)[i].pname_length, \
									(mysqlnd_qc_handler_read_t)(b)[i].r_func, (mysqlnd_qc_handler_write_t)(b)[i].w_func TSRMLS_CC); \
		i++; \
	}\
}

#define MYSQLND_QC_ADD_PROPERTIES_INFO(a,b) \
{ \
	unsigned int i = 0; \
	while (b[i].name != NULL) { \
		zend_declare_property_null((a), (b)[i].name, (b)[i].name_length, (b)[i].flags TSRMLS_CC); \
		i++; \
	}\
}


#define MYSQLND_QC_HANDLER_MAP_PROPERTY_LONG(__func, __handler_type, __field)\
static int __func(mysqlnd_qc_handler_object *obj, zval **retval TSRMLS_DC) \
{\
	__handler_type * __handler = (__handler_type *) obj->ptr; \
	MAKE_STD_ZVAL(*retval); \
	if (!__handler) { \
		ZVAL_NULL(*retval); \
	} else {\
		ZVAL_LONG(*retval, __handler->__field); \
	} \
	return SUCCESS; \
}

#define MYSQLND_QC_HANDLER_MAP_PROPERTY_STRING(__func, __handler_type, __field)\
static int __func(mysqlnd_qc_handler_object *obj, zval **retval TSRMLS_DC) \
{\
	__handler_type * __handler = (__handler_type *) obj->ptr; \
	MAKE_STD_ZVAL(*retval); \
	if (!__handler || !__handler->__field) { \
		ZVAL_NULL(*retval); \
	} else {\
		ZVAL_STRING(*retval, __handler->__field, 1); \
	} \
	return SUCCESS; \
}



extern HashTable mysqlnd_qc_classes;

PHP_MYSQLND_QC_EXPORT(zend_object_value) mysqlnd_qc_handler_objects_new(zend_class_entry *class_type TSRMLS_DC);
void mysqlnd_qc_handler_add_property(HashTable *h, const char *pname, size_t pname_len, mysqlnd_qc_handler_read_t r_func, mysqlnd_qc_handler_write_t w_func TSRMLS_DC);

void mysqlnd_qc_handler_classes_minit(TSRMLS_D);
void mysqlnd_qc_handler_classes_mshutdown(TSRMLS_D);


#endif /* MYSQLND_QC_CLASSES_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
