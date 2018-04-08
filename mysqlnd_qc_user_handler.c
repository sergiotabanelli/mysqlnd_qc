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
#include "zend_llist.h"
#include "php_mysqlnd_qc.h"
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_zval_util.h"
#include "mysqlnd_qc_std_handler.h"
#include "mysqlnd_qc_user_handler.h"
#include "mysqlnd_qc_classes.h"

/* {{{ mysqlnd_qc_call_handler */
static zval *
mysqlnd_qc_call_handler(zval *func, int argc, zval **argv, zend_bool destroy_args TSRMLS_DC)
{
	int i;
	zval * retval;
	DBG_ENTER("mysqlnd_qc_call_handler");

	MAKE_STD_ZVAL(retval);
	if (call_user_function(EG(function_table), NULL, func, retval, argc, argv TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Failed to call '%s'", MYSQLND_QC_ERROR_PREFIX, Z_STRVAL_P(func));
		zval_ptr_dtor(&retval);
		retval = NULL;
	}

	if (destroy_args == TRUE) {
		for (i = 0; i < argc; i++) {
			zval_ptr_dtor(&argv[i]);
		}
	}

	DBG_RETURN(retval);
}
/* }}} */



/* {{{ mysqlnd_qc_user_get_hash_key */
static char *
mysqlnd_qc_user_get_hash_key(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	zval * args[7];
	zval * retval;
	char * query_hash_key = NULL;

	DBG_ENTER("mysqlnd_qc_user_get_hash_key");

	QC_STRING((char*)conn->host_info, args[0]);
	QC_LONG(conn->port, args[1]);
	QC_LONG(((conn->charset) ? conn->charset->nr : 0), args[2]);
	QC_STRING((char*)conn->user, args[3]);
	QC_STRING((char*)conn->connect_or_select_db? conn->connect_or_select_db:"", args[4]);
	QC_STRINGL((char*)query, query_len, args[5]);
	QC_STRINGL((char*)server_id, server_id_len, args[6]);

	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.get_hash, 7, args, TRUE TSRMLS_CC);
	if (!retval) {
		query_hash_key = "";
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


/* {{{ mysqlnd_qc_user_find_query_in_cache */
static smart_str *
mysqlnd_qc_user_find_query_in_cache(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC)
{
	smart_str * cached_query = NULL;
	zval * args[1];
	zval * retval;

	DBG_ENTER("mysqlnd_qc_user_find_query_in_cache");
	DBG_INF_FMT("hash=%s", query_hash_key);

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);

	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.find_in_cache, 1, args, TRUE TSRMLS_CC);
	if (retval){
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


/* {{{ mysqlnd_qc_user_return_to_cache */
static void
mysqlnd_qc_user_return_to_cache(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC)
{
	zval * args[2];
	zval * retval;

	DBG_ENTER("mysqlnd_qc_user_return_to_cache");

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);
	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.return_to_cache, 1, args, TRUE TSRMLS_CC);
	if (retval){
		zval_ptr_dtor(&retval);
	}
	smart_str_free_ex(cached_query, PERSISTENT_STR);
	mnd_free(cached_query);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_user_add_query_to_cache_if_not_exists */
static enum_func_status
mysqlnd_qc_user_add_query_to_cache_if_not_exists(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len,
												smart_str * recorded_data, uint TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	zval * args[6];
	zval * retval;
	DBG_ENTER("mysqlnd_qc_user_add_query_to_cache_if_not_exists");

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);
	QC_STRINGL((char*)recorded_data->c, recorded_data->len, args[1]);
	QC_LONG(TTL, args[2]);
	QC_LONG(run_time, args[3]);
	QC_LONG(store_time, args[4]);
	QC_LONG(row_count, args[5]);

	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.add_to_cache, 6, args, TRUE TSRMLS_CC);
	if (retval) {
		convert_to_boolean(retval);
		ret = (TRUE == Z_BVAL_P(retval)) ? PASS : FAIL;
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


/* TODO: copied from MS */
/* {{{ mysqlnd_qc_match_wild */
zend_bool mysqlnd_qc_match_wild(const char * const str, const char * const wildstr TSRMLS_DC)
{
	static char many = '%';
	static char single = '_';
	static char escape = '\\';
	const char * s = str;
	const char * w = wildstr;

	DBG_ENTER("mysqlnd_qc_match_wild");
	/* check for */
	if (!s || !w) {
		DBG_RETURN(FALSE);
	}
	do {
		while (*w != many && *w != single) {
			if (*w == escape && !*++w) {
				DBG_RETURN(FALSE);
			}
			if (*s != *w) {
				DBG_RETURN(FALSE);
			} else if (!*s) {
				/* the same chars, and both are \0 terminators */
				DBG_RETURN(TRUE);
			}
			/* still not the end */
			++s;
			++w;
		}
		/* one or many */
		if (*w == many) {
			/* even if *s is \0 this is ok */
			DBG_RETURN(TRUE);
		} else if (*w == single) {
			if (!*s) {
				/* single is not zero */
				DBG_RETURN(FALSE);
			}
			++s;
			++w;
		}
	} while (1);
}
/* }}} */


/* {{{ mysqlnd_qc_user_should_cache()
 */
zend_bool
mysqlnd_qc_user_should_cache(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len, smart_str * recorded_data, uint * TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	zend_bool ret = TRUE;
	zend_llist_position	pos_entry;
	MYSQLND_QC_SHOULD_CACHE_CONDITION * entry;

	DBG_ENTER("mysqlnd_qc_user_should_cache");

	if (!result) {
		DBG_INF("No result");
		DBG_RETURN(ret);
	}

	for (entry = (MYSQLND_QC_SHOULD_CACHE_CONDITION *) zend_llist_get_first_ex(&MYSQLND_QC_G(should_cache_conditions), &pos_entry);
					entry;
					entry = (MYSQLND_QC_SHOULD_CACHE_CONDITION *) zend_llist_get_next_ex(&MYSQLND_QC_G(should_cache_conditions), &pos_entry)) {
		if ((QC_CONDITION_META_SCHEMA_PATTERN == entry->type) &&
				(entry->conditions) &&
				(zend_llist_count((zend_llist *)entry->conditions) > 0))
		{
			MYSQLND_QC_CONDITION_PATTERN * cond;
			zend_llist_position	pos_cond;
			int i;

			ret = FALSE;
			for (cond = (MYSQLND_QC_CONDITION_PATTERN *) zend_llist_get_first_ex((zend_llist *)entry->conditions, &pos_cond);
					cond && !ret;
					cond = (MYSQLND_QC_CONDITION_PATTERN *) zend_llist_get_next_ex((zend_llist *)entry->conditions, &pos_cond)) {
				for (i = 0; (i < mysqlnd_num_fields(result)) && !ret; i++) {
					const MYSQLND_FIELD *field = mysqlnd_fetch_field_direct((MYSQLND_RES * const)result, i);
					char * qualified_name;
					spprintf(&qualified_name, 0, "%s.%s", field->db, field->org_table);
					if (TRUE == mysqlnd_qc_match_wild(qualified_name, cond->pattern TSRMLS_CC)) {
						if (cond->ttl) {
							*TTL = cond->ttl;
						}
						ret = TRUE;
						efree(qualified_name);
						break;
					}
					efree(qualified_name);
				}
			}
			break;
		}
	}

	DBG_INF_FMT("ret=%s", ret == TRUE? "TRUE":"FALSE");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_query_is_select */
PHP_MYSQLND_QC_API zend_bool
mysqlnd_qc_query_is_select(const char * query, size_t query_len, uint * ttl, char ** server_id, size_t * server_id_len TSRMLS_DC)
{
	zval * args[1];
	zval * retval;
	zend_bool ret = FALSE;

	DBG_ENTER("mysqlnd_qc_query_is_select");
	DBG_INF_FMT("query(50bytes)=%*s query_is_select=%p", MIN(50, query_len), query, MYSQLND_QC_G(query_is_select));

	if (MYSQLND_QC_G(query_is_select)) {
		*ttl = 0;

		/* TODO: support setting server id by the user */
		QC_STRINGL((char*)query, query_len, args[0]);
		retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(query_is_select), 1, args, TRUE TSRMLS_CC);
		if (retval) {
			if (Z_TYPE_P(retval) == IS_BOOL) {
				ret = (TRUE == Z_BVAL_P(retval)) ? TRUE : FALSE;
			} else if (Z_TYPE_P(retval) == IS_ARRAY) {
				zval ** zv_ttl;
				zval ** zv_server_id;

				*server_id = NULL;
				*server_id_len = 0;

				if (FAILURE == zend_hash_find(Z_ARRVAL_P(retval), "ttl", sizeof("ttl"), (void**) &zv_ttl)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Return value is borked. TTL is missing",
									 MYSQLND_QC_ERROR_PREFIX
									);
					ret = FALSE;
				} else {
					convert_to_long_ex(zv_ttl);
					if (Z_LVAL_PP(zv_ttl) >= 0) {
						ret = TRUE;
						*ttl = Z_LVAL_PP(zv_ttl);
					} else {
						ret = FALSE;
					}
				}

				if (FAILURE == zend_hash_find(Z_ARRVAL_P(retval), "server_id", sizeof("server_id"), (void**) &zv_server_id)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Return value is borked. server_id is missing", MYSQLND_QC_ERROR_PREFIX);
					ret = FALSE;
				} else {
					ret = TRUE;
					if (IS_STRING == Z_TYPE_PP(zv_server_id)) {
						*server_id_len = spprintf(server_id, 0, "%s", Z_STRVAL_PP(zv_server_id));
					}
				}
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Return value must be boolean or an array", MYSQLND_QC_ERROR_PREFIX);
				ret = FALSE;
			}
			zval_ptr_dtor(&retval);
		}
	} else {
		/* if no is_select is set by the user use the default one */
		ret = mysqlnd_qc_handler_default_query_is_select(query, query_len, ttl, server_id, server_id_len TSRMLS_CC);
	}

	DBG_INF_FMT("query_is_select=%d ttl=%d server_id=%s", ret, *ttl, *server_id);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_user_update_cache_stats */
static void
mysqlnd_qc_user_update_cache_stats(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC)
{
	zval * args[3];
	zval * retval;
	DBG_ENTER("mysqlnd_qc_user_update_cache_stats");

	QC_STRINGL((char*)query_hash_key, query_hash_key_len, args[0]);
	QC_LONG(run_time, args[1]);
	QC_LONG(store_time, args[2]);

	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.update_query_run_time_stats, 3, args, TRUE TSRMLS_CC);
	if (retval) {
		zval_ptr_dtor(&retval);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_user_default_fill_stats_hash */
static long
mysqlnd_qc_user_default_fill_stats_hash(zval *return_value TSRMLS_DC ZEND_FILE_LINE_DC)
{
	zval * retval;
	DBG_ENTER("mysqlnd_qc_user_default_fill_stats_hash");
	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.fill_stats_hash, 0, NULL, TRUE TSRMLS_CC);
	if (retval) {
		*return_value = *retval;
		zval_copy_ctor(return_value);
		convert_to_array(return_value);
		zval_ptr_dtor(&retval);
		DBG_RETURN(zend_hash_num_elements(Z_ARRVAL_P(return_value)));
	}
	DBG_RETURN(0);
}
/* }}} */


/* {{{ mysqlnd_qc_user_clear_cache */
static enum_func_status
mysqlnd_qc_user_clear_cache(TSRMLS_D)
{
	zval * retval;
	zend_bool ret = FALSE;

	DBG_ENTER("mysqlnd_qc_user_clear_cache");

	retval = mysqlnd_qc_call_handler(MYSQLND_QC_G(user_handlers_names).name.clear_cache, 0, NULL, FALSE TSRMLS_CC);
	if (retval) {
		convert_to_boolean(retval);
		ret = Z_BVAL_P(retval);
		zval_ptr_dtor(&retval);
	}

	DBG_RETURN(ret == TRUE? PASS:FAIL);
}
/* }}} */


/* {{{ mysqlnd_qc_user_handler_change_init */
static enum_func_status
mysqlnd_qc_user_handler_change_init(TSRMLS_D)
{
	enum_func_status ret = PASS;
	int i = 0;
	DBG_ENTER("mysqlnd_qc_user_handler_change_shutdown");

	for (i = 0; i < NUM_USER_HANDLERS; i++) {
		Z_ADDREF_P(MYSQLND_QC_G(user_handlers_names).names[i]);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_user_handler_change_shutdown */
static enum_func_status
mysqlnd_qc_user_handler_change_shutdown(TSRMLS_D)
{
	enum_func_status ret = PASS;
	int i = 0;
	DBG_ENTER("mysqlnd_qc_user_handler_change_shutdown");

	for (i = 0; i < NUM_USER_HANDLERS; i++) {
		zval_ptr_dtor(&MYSQLND_QC_G(user_handlers_names).names[i]);
		MYSQLND_QC_G(user_handlers_names).names[i] = NULL;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_user_handler_change_refresh */
static enum_func_status
mysqlnd_qc_user_handler_change_refresh(TSRMLS_D)
{
	enum_func_status ret = PASS;
	int i = 0;
	DBG_ENTER("mysqlnd_qc_user_handler_change_refresh");

	for (i = 0; i < NUM_USER_HANDLERS; i++) {
		Z_ADDREF_P(MYSQLND_QC_G(user_handlers_names).names[i]);
	}

	DBG_RETURN(ret);
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_user) = {
	MYSQLND_QC_USER_HANDLER_NAME,
	MYSQLND_QC_USER_HANDLER_VERSION_STR,
	mysqlnd_qc_user_get_hash_key,
	NULL, /* query is cached */
	mysqlnd_qc_user_find_query_in_cache,
	mysqlnd_qc_user_return_to_cache,
	mysqlnd_qc_user_add_query_to_cache_if_not_exists,
	mysqlnd_qc_user_update_cache_stats,
	mysqlnd_qc_user_default_fill_stats_hash, /* fill_stats_hash */
	mysqlnd_qc_user_clear_cache, /* clear_cache */
	NULL,
	NULL,
	mysqlnd_qc_user_handler_change_init, /* handler_change_init */
	mysqlnd_qc_user_handler_change_shutdown,  /* handler_change_shutdown */
	mysqlnd_qc_user_handler_change_refresh  /* handler_change_refresh */
MYSQLND_CLASS_METHODS_END;


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
