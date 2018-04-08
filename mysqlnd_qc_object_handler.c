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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_mysqlnd_qc.h"
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_zval_util.h"
#include "mysqlnd_qc_std_handler.h"
#include "mysqlnd_qc_object_handler.h"
#include "mysqlnd_qc_classes.h"


#define call_method_0_param(obj_p, fn_proxy, fn_name, retval_p)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL TSRMLS_CC)

#define call_method_1_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 1, (args)[0], NULL, NULL, NULL, NULL, NULL, NULL TSRMLS_CC)

#define call_method_2_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 2, (args)[0], (args)[1], NULL, NULL, NULL, NULL, NULL TSRMLS_CC)

#define call_method_3_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 3, (args)[0], (args)[1], (args)[2], NULL, NULL, NULL, NULL TSRMLS_CC)

#define call_method_4_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 4, (args)[0], (args)[1], (args)[2], (args)[3], NULL, NULL, NULL TSRMLS_CC)

#define call_method_5_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 5, (args)[0], (args)[1], (args)[2], (args)[3], (args)[4], NULL, NULL TSRMLS_CC)

#define call_method_6_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 6, (args)[0], (args)[1], (args)[2], (args)[3], (args)[4], (args)[5], NULL TSRMLS_CC)

#define call_method_7_param(obj_p, fn_proxy, fn_name, retval_p, args)  \
	mysqlnd_qc_call_method(&(obj_p), Z_OBJCE_P((obj_p)), (fn_proxy), fn_name, sizeof(fn_name) - 1, &(retval_p), 7, (args)[0], (args)[1], (args)[2], (args)[3], (args)[4], (args)[5], (args)[6] TSRMLS_CC)



/* {{{ mysqlnd_qc_call_method
 Only returns the returned zval if retval_ptr != NULL */
static zval *
mysqlnd_qc_call_method(zval **object_pp, zend_class_entry *obj_ce, zend_function **fn_proxy, char *function_name, int function_name_len,
					   zval **retval_ptr_ptr, int param_count, zval* arg1, zval* arg2, zval* arg3, zval* arg4, zval* arg5, zval* arg6, zval* arg7 TSRMLS_DC)
{
	int result;
	zend_fcall_info fci;
	zval z_fname;
	zval *retval;
	HashTable *function_table;

	zval **params[7];
	DBG_ENTER("mysqlnd_qc_call_method");

	params[0] = &arg1;
	params[1] = &arg2;
	params[2] = &arg3;
	params[3] = &arg4;
	params[4] = &arg5;
	params[5] = &arg6;
	params[6] = &arg7;

	DBG_INF_FMT("param_count=%d function=%s", param_count, function_name);

	fci.size = sizeof(fci);
	/*fci.function_table = NULL; will be read form zend_class_entry of object if needed */
	fci.object_ptr = object_pp ? *object_pp : NULL;
	fci.function_name = &z_fname;
	fci.retval_ptr_ptr = retval_ptr_ptr ? retval_ptr_ptr : &retval;
	fci.param_count = param_count;
	fci.params = params;
	fci.no_separation = 1;
	fci.symbol_table = NULL;

	if (!fn_proxy && !obj_ce) {
		/* no interest in caching and no information already present that is
		 * needed later inside zend_call_function. */
		ZVAL_STRINGL(&z_fname, function_name, function_name_len, 0);
		fci.function_table = !object_pp ? EG(function_table) : NULL;
		result = zend_call_function(&fci, NULL TSRMLS_CC);
	} else {
		zend_fcall_info_cache fcic;

		fcic.initialized = 1;
		if (!obj_ce) {
			obj_ce = object_pp ? Z_OBJCE_PP(object_pp) : NULL;
		}
		if (obj_ce) {
			function_table = &obj_ce->function_table;
		} else {
			function_table = EG(function_table);
		}
		if (!fn_proxy || !*fn_proxy) {
			if (zend_hash_find(function_table, function_name, function_name_len+1, (void **) &fcic.function_handler) == FAILURE) {
				/* error at c-level */
				zend_error(E_CORE_ERROR, "Couldn't find implementation for method %s%s%s", obj_ce ? obj_ce->name : "", obj_ce ? "::" : "", function_name);
			}
			if (fn_proxy) {
				*fn_proxy = fcic.function_handler;
			}
		} else {
			fcic.function_handler = *fn_proxy;
		}
		fcic.calling_scope = obj_ce;
		if (object_pp) {
			fcic.called_scope = Z_OBJCE_PP(object_pp);
		} else if (obj_ce &&
		           !(EG(called_scope) &&
		             instanceof_function(EG(called_scope), obj_ce TSRMLS_CC))) {
			fcic.called_scope = obj_ce;
		} else {
			fcic.called_scope = EG(called_scope);
		}
		fcic.object_ptr = object_pp ? *object_pp : NULL;
		result = zend_call_function(&fci, &fcic TSRMLS_CC);
	}
	if (result == FAILURE) {
		/* error at c-level */
		if (!obj_ce) {
			obj_ce = object_pp ? Z_OBJCE_PP(object_pp) : NULL;
		}
		if (!EG(exception)) {
			zend_error(E_CORE_ERROR, "Couldn't execute method %s%s%s", obj_ce ? obj_ce->name : "", obj_ce ? "::" : "", function_name);
		}
	}
	/* now release the params */
	{
		int i = 0;
		for (; i < param_count; i++) {
			zval_ptr_dtor(params[i]);
		}
	}


	if (!retval_ptr_ptr) {
		if (retval) {
			zval_ptr_dtor(&retval);
		}
		DBG_RETURN(NULL);
	}
	DBG_RETURN(*retval_ptr_ptr);
}
/* }}} */


/* {{{ mysqlnd_qc_object_get_hash_key */
static char *
mysqlnd_qc_object_get_hash_key(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	zval * args[7];
	zval * retval = NULL;
	char * query_hash_key = NULL;

	DBG_ENTER("mysqlnd_qc_object_get_hash_key");

	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_RETURN(query_hash_key);
	}

	QC_STRING((char*)conn->host_info, args[0]);
	QC_LONG(conn->port, args[1]);
	QC_LONG(((conn->charset) ? conn->charset->nr : 0), args[2]);
	QC_STRING((char*)conn->user, args[3]);
	QC_STRING((char*)conn->connect_or_select_db? conn->connect_or_select_db:"", args[4]);
	QC_STRINGL((char*)query, query_len, args[5]);
	QC_BOOL(persistent, args[6]);

	call_method_7_param(MYSQLND_QC_G(handler_object), NULL /*fn_proxy - don't cache */, MYSQLND_QC_GET_HASH_KEY_NAME, retval, args);

	if (!retval) {
		query_hash_key = pestrndup("", sizeof("") - 1, conn->persistent);
		*query_hash_key_len = 0;
	} else {
		convert_to_string(retval);
		if (conn->persistent) {
			query_hash_key = pemalloc(Z_STRLEN_P(retval) + 1, conn->persistent);
			memcpy(query_hash_key, Z_STRVAL_P(retval), Z_STRLEN_P(retval) + 1);
			*query_hash_key_len = Z_STRLEN_P(retval);
		} else {
			query_hash_key = Z_STRVAL_P(retval);
			*query_hash_key_len = Z_STRLEN_P(retval);
			ZVAL_NULL(retval);
		}
		zval_ptr_dtor(&retval);
	}

	if (0 == *query_hash_key_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Hash key is empty", MYSQLND_QC_ERROR_PREFIX);
	}

	DBG_INF_FMT("query_hash_key(%d)=%s", *query_hash_key_len, query_hash_key);
	DBG_RETURN(query_hash_key);
}
/* }}} */


/* {{{ mysqlnd_qc_object_find_query_in_cache */
static smart_str *
mysqlnd_qc_object_find_query_in_cache(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC)
{
	smart_str * cached_query = NULL;
	zval * args[1];
	zval * retval = NULL;

	DBG_ENTER("mysqlnd_qc_object_find_query_in_cache");
	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_RETURN(NULL);
	}

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);

	call_method_1_param(MYSQLND_QC_G(handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_FIND_IN_CACHE_NAME, retval, args);

	if (retval) {
		if (Z_TYPE_P(retval) != IS_NULL) {
			convert_to_string(retval);
			cached_query = mnd_calloc(1, sizeof(smart_str));
			smart_str_appendl_ex(cached_query, Z_STRVAL_P(retval), Z_STRLEN_P(retval) + 1, PERSISTENT_STR);
			DBG_INF_FMT("cached_len=%d", cached_query->len);
		}
		zval_ptr_dtor(&retval);
	}
	DBG_INF_FMT("cached_query=%p", cached_query);
	DBG_RETURN(cached_query);
}
/* }}} */


/* {{{ mysqlnd_qc_object_return_to_cache */
static void
mysqlnd_qc_object_return_to_cache(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC)
{
	zval * args[1];
	zval * retval = NULL;

	DBG_ENTER("mysqlnd_qc_object_return_to_cache");
	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_VOID_RETURN;
	}

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);

	call_method_1_param(MYSQLND_QC_G(handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_RETURN_TO_CACHE_NAME, retval, args);

	if (retval) {
		zval_ptr_dtor(&retval);
	}
	smart_str_free_ex(cached_query, PERSISTENT_STR);
	mnd_free(cached_query);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_object_add_query_to_cache_if_not_exists */
static enum_func_status
mysqlnd_qc_object_add_query_to_cache_if_not_exists(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len,
												smart_str * recorded_data, uint TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	zval * args[6];
	zval * retval = NULL;
	DBG_ENTER("mysqlnd_qc_object_add_query_to_cache_if_not_exists");

	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_RETURN(FAIL);
	}

	DBG_INF_FMT("Recorded data len = %d", recorded_data->len);

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);
	QC_STRINGL((char*)recorded_data->c, recorded_data->len, args[1]);
	QC_LONG(TTL, args[2]);
	QC_LONG(run_time, args[3]);
	QC_LONG(store_time, args[4]);
	QC_LONG(row_count, args[5]);
	DBG_INF_FMT("Recorded data zval len = %d", Z_STRLEN_P(args[1]));

	call_method_6_param(MYSQLND_QC_G(handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_ADD_TO_CACHE_NAME, retval, args);

	if (retval) {
		convert_to_boolean(retval);
		ret = Z_BVAL_P(retval) ? PASS : FAIL;
		zval_ptr_dtor(&retval);
		if (ret == PASS) {
			smart_str_free_ex(recorded_data, PERSISTENT_STR);
			mnd_free(recorded_data);
		}
	}
	DBG_INF_FMT("ret=%s", ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_object_update_cache_stats */
static void
mysqlnd_qc_object_update_cache_stats(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC)
{
	zval * args[3];
	zval * retval = NULL;
	DBG_ENTER("mysqlnd_qc_object_update_cache_stats");

	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_VOID_RETURN;
	}

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);
	QC_LONG(run_time, args[1]);
	QC_LONG(store_time, args[2]);

	call_method_3_param(MYSQLND_QC_G(handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_UPDATE_CACHE_STATS_NAME, retval, args);
	if (retval) {
		zval_ptr_dtor(&retval);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_object_clear_cache */
static enum_func_status
mysqlnd_qc_object_clear_cache(TSRMLS_D)
{
	zval * retval;
	zend_bool ret = FALSE;

	DBG_ENTER("mysqlnd_qc_object_clear_cache");
	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_RETURN(FAIL);
	}

	call_method_0_param(MYSQLND_QC_G(handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_CLEAR_CACHE_NAME, retval);
	if (retval) {
		convert_to_boolean(retval);
		ret = Z_BVAL_P(retval) ? TRUE : FALSE;
		zval_ptr_dtor(&retval);
	}

	DBG_RETURN(ret == TRUE? PASS:FAIL);
}
/* }}} */


/* {{{ mysqlnd_qc_object_handler_change_init */
static enum_func_status
mysqlnd_qc_object_handler_change_init(TSRMLS_D)
{
	zval * retval;
	enum_func_status ret = FAIL;
	DBG_ENTER("mysqlnd_qc_object_handler_init");

	if (!MYSQLND_QC_G(new_handler_object)) {
		DBG_ERR("new_handler_object is NULL");
		DBG_RETURN(ret);
	}

	call_method_0_param(MYSQLND_QC_G(new_handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_INIT_NAME, retval);
	if (retval) {
		convert_to_boolean(retval);
		ret = Z_BVAL_P(retval) ? PASS : FAIL;
		zval_ptr_dtor(&retval);
		if (ret == PASS) {
			MYSQLND_QC_G(handler_object) = MYSQLND_QC_G(new_handler_object);
			Z_ADDREF_P(MYSQLND_QC_G(handler_object));
			MYSQLND_QC_G(new_handler_object) = NULL;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_object_handler_shutdown */
static enum_func_status
mysqlnd_qc_object_handler_change_shutdown(TSRMLS_D)
{
	zval * retval;
	zend_bool ret = FAIL;
	DBG_ENTER("mysqlnd_qc_object_handler_shutdown");
	/* ToDo : real shutdown */

	if (!MYSQLND_QC_G(handler_object)) {
		DBG_ERR("handler_object is NULL");
		DBG_RETURN(ret);
	}

	if (zend_is_executing(TSRMLS_C)) {
		call_method_0_param(MYSQLND_QC_G(handler_object), NULL /* fn_proxy - don't cache */, MYSQLND_QC_SHUTDOWN_NAME, retval);
		if (retval) {
			convert_to_boolean(retval);
			ret = Z_BVAL_P(retval)? PASS:FAIL;
			zval_ptr_dtor(&retval);
		}
	}

	zval_ptr_dtor(&MYSQLND_QC_G(handler_object));
	MYSQLND_QC_G(handler_object) = NULL;
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_object_handler_change_refresh */
static enum_func_status
mysqlnd_qc_object_handler_change_refresh(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_object_handler_change_refresh");
	DBG_RETURN(mysqlnd_qc_object_handler_change_shutdown(TSRMLS_C) == PASS && mysqlnd_qc_object_handler_change_init(TSRMLS_C) == PASS ? PASS:FAIL);
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_object) = {
	MYSQLND_QC_OBJECT_HANDLER_NAME,
	MYSQLND_QC_OBJECT_HANDLER_VERSION_STR,
	mysqlnd_qc_object_get_hash_key,
	NULL,
	mysqlnd_qc_object_find_query_in_cache,
	mysqlnd_qc_object_return_to_cache,
	mysqlnd_qc_object_add_query_to_cache_if_not_exists,
	mysqlnd_qc_object_update_cache_stats,
	NULL,	/* fill_stats_hash */
	mysqlnd_qc_object_clear_cache, /* clear_cache */
	NULL,	/* minit */
	NULL,	/* mshutdown */
	mysqlnd_qc_object_handler_change_init,
	mysqlnd_qc_object_handler_change_shutdown,
	mysqlnd_qc_object_handler_change_refresh
MYSQLND_CLASS_METHODS_END;


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
