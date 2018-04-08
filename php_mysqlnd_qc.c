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
#include "mysqlnd_qc_classes.h"
#include "mysqlnd_qc_logs.h"

/* versions */
#include "mysqlnd_qc_std_handler.h"
#include "mysqlnd_qc_nop_handler.h"
#include "mysqlnd_qc_user_handler.h"
#include "mysqlnd_qc_object_handler.h"
#ifdef MYSQLND_QC_HAVE_APC
#include "mysqlnd_qc_apc_handler.h"
#endif
#ifdef MYSQLND_QC_HAVE_MEMCACHE
#include "mysqlnd_qc_memcache_handler.h"
#endif
#ifdef MYSQLND_QC_HAVE_SQLITE
#include "mysqlnd_qc_sqlite_handler.h"
#endif

#ifdef PHP_WIN32
#include "win32/time.h"
#else
#include "sys/time.h"
#endif
#include <stdlib.h>

ZEND_DECLARE_MODULE_GLOBALS(mysqlnd_qc)

struct st_mysqlnd_qc_methods *mysqlnd_qc_methods = &MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_std);

struct st_mysqlnd_qc_qcache	norm_query_trace_log;

static struct st_mysqlnd_qc_methods * mysqlnd_qc_handlers[] =
{
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_std),
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_nop),
#ifdef MYSQLND_QC_HAVE_APC
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_apc),
#endif
#ifdef MYSQLND_QC_HAVE_MEMCACHE
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_memcache),
#endif
#ifdef MYSQLND_QC_HAVE_SQLITE
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_sqlite),
#endif
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_user),
	&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_object)
};

#define STR_W_LEN(str)  str, (sizeof(str) - 1)
const MYSQLND_STRING mysqlnd_qc_stats_values_names[QC_STAT_LAST] =
{
	{ STR_W_LEN("cache_hit") },
	{ STR_W_LEN("cache_miss") },
	{ STR_W_LEN("cache_put") },
	{ STR_W_LEN("query_should_cache") },
	{ STR_W_LEN("query_should_not_cache") },
	{ STR_W_LEN("query_not_cached") },
	{ STR_W_LEN("query_could_cache") },
	{ STR_W_LEN("query_found_in_cache") },
	{ STR_W_LEN("query_uncached_other") },
	{ STR_W_LEN("query_uncached_no_table") },
	{ STR_W_LEN("query_uncached_no_result") },
	{ STR_W_LEN("query_uncached_use_result") },
	{ STR_W_LEN("query_aggr_run_time_cache_hit") },
	{ STR_W_LEN("query_aggr_run_time_cache_put") },
	{ STR_W_LEN("query_aggr_run_time_total") },
	{ STR_W_LEN("query_aggr_store_time_cache_hit") },
	{ STR_W_LEN("query_aggr_store_time_cache_put") },
	{ STR_W_LEN("query_aggr_store_time_total") },
	{ STR_W_LEN("receive_bytes_recorded") },
	{ STR_W_LEN("receive_bytes_replayed") },
	{ STR_W_LEN("send_bytes_recorded") },
	{ STR_W_LEN("send_bytes_replayed") },
	{ STR_W_LEN("slam_stale_refresh") },
	{ STR_W_LEN("slam_stale_hit") }
};
/* }}} */


#ifdef COMPILE_DL_MYSQLND_QC
ZEND_GET_MODULE(mysqlnd_qc)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("mysqlnd_qc.enable_qc", "1", PHP_INI_SYSTEM, OnUpdateBool, enable_qc, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.ttl", "30", PHP_INI_ALL, OnUpdateLongGEZero, ttl, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.cache_by_default", "0", PHP_INI_ALL, OnUpdateBool, cache_by_default, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.cache_no_table", "0", PHP_INI_ALL, OnUpdateBool, cache_no_table, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.use_request_time", "0", PHP_INI_ALL, OnUpdateBool, use_request_time, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.time_statistics", "1", PHP_INI_ALL, OnUpdateBool, time_statistics, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.collect_statistics", "0", PHP_INI_ALL, OnUpdateBool, collect_statistics, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.collect_statistics_log_file", "/tmp/mysqlnd_qc.stats", PHP_INI_SYSTEM, OnUpdateString, collect_statistics_log_file, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.collect_query_trace", "0", PHP_INI_SYSTEM, OnUpdateBool, collect_query_trace, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.query_trace_bt_depth", "3", PHP_INI_SYSTEM, OnUpdateLongGEZero, query_trace_bt_depth, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.slam_defense", "0", PHP_INI_SYSTEM, OnUpdateBool, slam_defense, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.slam_defense_ttl", "30", PHP_INI_SYSTEM, OnUpdateLongGEZero, slam_defense_ttl, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
#ifdef NORM_QUERY_TRACE_LOG
	STD_PHP_INI_ENTRY("mysqlnd_qc.collect_normalized_query_trace", "0", PHP_INI_SYSTEM, OnUpdateBool, collect_normalized_query_trace, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
#endif
	STD_PHP_INI_ENTRY("mysqlnd_qc.std_data_copy", "0", PHP_INI_SYSTEM, OnUpdateBool, std_data_copy, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
#ifdef MYSQLND_QC_HAVE_APC
	STD_PHP_INI_ENTRY("mysqlnd_qc.apc_prefix", "qc_", PHP_INI_ALL, OnUpdateStringUnempty, apc_prefix, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
#endif
#ifdef MYSQLND_QC_HAVE_MEMCACHE
	STD_PHP_INI_ENTRY("mysqlnd_qc.memc_server", "127.0.0.1", PHP_INI_ALL, OnUpdateStringUnempty, memc_server, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
	STD_PHP_INI_ENTRY("mysqlnd_qc.memc_port", "11211", PHP_INI_ALL, OnUpdateLongGEZero, memc_port, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
#endif
#ifdef MYSQLND_QC_HAVE_SQLITE
	STD_PHP_INI_ENTRY("mysqlnd_qc.sqlite_data_file", ":memory:", PHP_INI_ALL, OnUpdateStringUnempty, sqlite_data_file, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
#endif
	STD_PHP_INI_ENTRY("mysqlnd_qc.ignore_sql_comments", "1", PHP_INI_ALL, OnUpdateBool, ignore_sql_comments, zend_mysqlnd_qc_globals, mysqlnd_qc_globals)
PHP_INI_END()
/* }}} */


/* {{{ php_mysqlnd_qc_init_globals */
static void
php_mysqlnd_qc_init_globals(zend_mysqlnd_qc_globals *mysqlnd_qc_globals)
{
	mysqlnd_qc_globals->enable_qc = 1;
	mysqlnd_qc_globals->ttl = 30;
	mysqlnd_qc_globals->cache_by_default = FALSE;
	mysqlnd_qc_globals->cache_no_table = FALSE;
	mysqlnd_qc_globals->use_request_time = FALSE;
	mysqlnd_qc_globals->time_statistics = TRUE;
	mysqlnd_qc_globals->collect_statistics = FALSE;
	mysqlnd_qc_globals->collect_statistics_log_file = NULL;
	mysqlnd_qc_globals->collect_query_trace = FALSE;
	mysqlnd_qc_globals->query_trace_bt_depth = 3;
	mysqlnd_qc_globals->slam_defense = FALSE;
	mysqlnd_qc_globals->slam_defense_ttl = 30;
	mysqlnd_qc_globals->query_is_select = NULL;
#ifdef NORM_QUERY_TRACE_LOG
	mysqlnd_qc_globals->collect_normalized_query_trace = FALSE;
#endif
	mysqlnd_qc_globals->std_data_copy = FALSE;
	mysqlnd_qc_globals->handler_object = NULL;
#ifdef MYSQLND_QC_HAVE_APC
	mysqlnd_qc_globals->apc_prefix = "qc_";
#endif
#ifdef MYSQLND_QC_HAVE_MEMCACHE
	mysqlnd_qc_globals->memc_server = "127.0.0.1";
	mysqlnd_qc_globals->memc_port = 11211;
#endif
#ifdef MYSQLND_QC_HAVE_SQLITE
	mysqlnd_qc_globals->sqlite_data_file = ":memory:";
#endif
	mysqlnd_qc_globals->request_counter = 1;
	mysqlnd_qc_globals->ignore_sql_comments = 1;
}
/* }}} */


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mysqlnd_qc)
{
	php_mysqlnd_qc_init_globals(mysqlnd_qc_globals);
}
/* }}} */


/* {{{ should_cache_conditions_dtor */
static void
should_cache_conditions_dtor(void * pDest)
{
	MYSQLND_QC_SHOULD_CACHE_CONDITION * entry = (MYSQLND_QC_SHOULD_CACHE_CONDITION *)pDest;
	TSRMLS_FETCH();

	if ((QC_CONDITION_META_SCHEMA_PATTERN == entry->type) && entry->conditions) {
		zend_llist_destroy((zend_llist*)entry->conditions);
		mnd_efree(entry->conditions);
	}
}
/* }}} */


/* {{{ should_cache_conditions_entry_dtor */
static void
should_cache_conditions_entry_dtor(void * pDest)
{
	MYSQLND_QC_CONDITION_PATTERN * entry = (MYSQLND_QC_CONDITION_PATTERN *)pDest;

	if (entry->pattern) {
		efree((void *)entry->pattern);
	}
}
/* }}} */


/* {{{ mysqlnd_qc_set_storage_handler */
static zend_bool
mysqlnd_qc_set_storage_handler(const char * handler, int handler_len TSRMLS_DC)
{
	uint i;
	MYSQLND_QC_METHODS * org_handler = MYSQLND_QC_G(handler);
	DBG_ENTER("mysqlnd_qc_set_storage_handler");
	DBG_INF_FMT("before: %s handler_object=%p", org_handler->name, MYSQLND_QC_G(handler_object));

	for (i = 0; i < sizeof(mysqlnd_qc_handlers) / sizeof(mysqlnd_qc_handlers[0]); i++) {
		if (!strcasecmp(handler, mysqlnd_qc_handlers[i]->name)) {
			if (org_handler == mysqlnd_qc_handlers[i]) {
				/* From the same to the same handler */
				if (org_handler->handler_change_refresh && (FAIL == org_handler->handler_change_refresh(TSRMLS_C))) {
					/* May be acceptable, if we can install a new handler */
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Refresh of handler '%s' failed", org_handler->name);
					DBG_RETURN(FALSE);
				}
			} else {
				/* From one to other handler */
				if (org_handler->handler_change_shutdown && (FAIL == org_handler->handler_change_shutdown(TSRMLS_C))) {
					/* May be acceptable, if we can install a new handler */
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Shutdown of previous handler '%s' failed", org_handler->name);
					DBG_RETURN(FALSE);
				}
				if (mysqlnd_qc_handlers[i]->handler_change_init && (FAIL == mysqlnd_qc_handlers[i]->handler_change_init(TSRMLS_C))) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error during changing handler. Init of '%s' failed", handler);
					MYSQLND_QC_G(handler) = &MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_nop);
					DBG_RETURN(FALSE);
				}
			}
			LOCK_QC_METHODS();
			MYSQLND_QC_G(handler) = mysqlnd_qc_handlers[i];
			UNLOCK_QC_METHODS();
			DBG_INF_FMT("after: %s handler_object=%p", org_handler->name, MYSQLND_QC_G(handler_object));
			DBG_RETURN(TRUE);
		}
	}
	php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "Unknown handler '%s'", handler);
	DBG_RETURN(FALSE);
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_set_user_handlers(string get_hash_key, string find_query_in_cache, string add_query_to_cache_if_not_exists, string return_to_cache, string update_cache_stats, string get_stats, string clear_cache)
   Sets user-level functions */
static PHP_FUNCTION(mysqlnd_qc_set_user_handlers)
{
	zval ***args = NULL;
	int i, num_args, argc = ZEND_NUM_ARGS();
	char *name;

	DBG_ENTER("zif_mysqlnd_qc_set_user_handlers");
	if (argc != NUM_USER_HANDLERS) {
		zend_wrong_param_count(TSRMLS_C);
		DBG_VOID_RETURN;
	}

	if (zend_parse_parameters(argc TSRMLS_CC, "+", &args, &num_args) == FAILURE) {
		DBG_VOID_RETURN;
	}

	for (i = 0; i < NUM_USER_HANDLERS; i++) {
		if (!zend_is_callable(*args[i], 0, &name TSRMLS_CC)) {
			efree(args);
			php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "Argument %d is not a valid callback", i+1);
			efree(name);
			RETVAL_FALSE;
			DBG_VOID_RETURN;
		}
		DBG_INF_FMT("name=%s", Z_STRVAL_P(*args[i]));
		efree(name);
	}

	for (i = 0; i < NUM_USER_HANDLERS; i++) {
		/* the same will be done by mysqlnd_qc_user_handler_change_shutdown(). However, we will use mysqlnd_qc_user_handler_change_refresh */
		if (MYSQLND_QC_G(user_handlers_names).names[i] != NULL) {
			zval_ptr_dtor(&MYSQLND_QC_G(user_handlers_names).names[i]);
		}
		MYSQLND_QC_G(user_handlers_names).names[i] = *args[i];
	}

	efree(args);
	RETVAL_TRUE;
	mysqlnd_qc_set_storage_handler("user", sizeof("user") - 1 TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_set_is_select(string is_select)
   Sets is_select function callback */
static PHP_FUNCTION(mysqlnd_qc_set_is_select)
{
	zval *arg = NULL;
	char *name;

	DBG_ENTER("zif_mysqlnd_qc_set_is_select");

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &arg) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (!zend_is_callable(arg, 0, &name TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "Argument is not a valid callback");
		efree(name);
		RETVAL_FALSE;
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("name=%s", name);
	efree(name);

	/* the same will be done by mysqlnd_qc_user_handler_change_shutdown(). However, we will use mysqlnd_qc_user_handler_change_refresh */
	if (MYSQLND_QC_G(query_is_select) != NULL) {
		zval_ptr_dtor(&MYSQLND_QC_G(query_is_select));
	}
	MYSQLND_QC_G(query_is_select) = arg;
	Z_ADDREF_P(arg);

	RETVAL_TRUE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_set_storage_handler(string handler)
   Change build-in storage handler */
static PHP_FUNCTION(mysqlnd_qc_set_storage_handler)
{
	zval *	handler;
	const char * handler_name;
	size_t handler_name_len;

	DBG_ENTER("zif_mysqlnd_qc_set_storage_handler");
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &handler) == FAILURE) {
		DBG_VOID_RETURN;
	}
	if (IS_STRING == Z_TYPE_P(handler)) {
		if (strcasecmp(Z_STRVAL_P(handler), "user")) {
			handler_name = Z_STRVAL_P(handler);
			handler_name_len = Z_STRLEN_P(handler);
			RETVAL_BOOL(mysqlnd_qc_set_storage_handler(handler_name, handler_name_len TSRMLS_CC));
		} else {
			php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "User handler can be set only with mysqlnd_qc_set_user_handlers");
			RETVAL_FALSE;
		}
	} else if (IS_OBJECT == Z_TYPE_P(handler)) {
		handler_name = "object";
		handler_name_len = sizeof("object") - 1;
		MYSQLND_QC_G(new_handler_object) = handler;
		RETVAL_BOOL(mysqlnd_qc_set_storage_handler(handler_name, handler_name_len TSRMLS_CC));
	} else {
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "1st parameter must be either handler name or handler object");
		RETVAL_FALSE;
	}

	DBG_INF_FMT("after: %s", MYSQLND_QC_G(handler)->name);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto array mysqlnd_qc_get_cache_info()
   Information on the current cache contents, if available */
static PHP_FUNCTION(mysqlnd_qc_get_cache_info)
{
	uint num_entries;
	MYSQLND_QC_METHODS * handler = MYSQLND_QC_G(handler);
	zval * data_array;

	DBG_ENTER("mysqlnd_qc_get_cache_info");
	if (zend_parse_parameters_none() == FAILURE) {
		DBG_VOID_RETURN;
	}

	array_init(return_value);

	MAKE_STD_ZVAL(data_array);
	if (handler->fill_stats_hash) {
		num_entries = handler->fill_stats_hash(data_array TSRMLS_CC ZEND_FILE_LINE_CC);
	} else {
		num_entries = 0;
		array_init(data_array);
	}
	mysqlnd_qc_add_to_array_long(return_value, "num_entries", strlen("num_entries"), num_entries TSRMLS_CC);

	mysqlnd_qc_add_to_array_string(return_value, "handler", strlen("handler"), handler->name, strlen(handler->name) TSRMLS_CC);
	mysqlnd_qc_add_to_array_string(return_value, "handler_version", strlen("handler_version"), handler->version, strlen(handler->version) TSRMLS_CC);

	mysqlnd_qc_add_to_array_zval(return_value, "data", strlen("data"), data_array TSRMLS_CC);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_get_handler*/
static PHP_FUNCTION(mysqlnd_qc_get_available_handlers)
{
	DBG_ENTER("mysqlnd_qc_get_handler");
	if (zend_parse_parameters_none() == FAILURE) {
		DBG_VOID_RETURN;
	}

	array_init(return_value);

	{
		zval * sub;
		MAKE_STD_ZVAL(sub);
		array_init(sub);
		add_assoc_string(sub, "version", estrdup(MYSQLND_QC_STD_HANDLER_VERSION_STR), 0);
		add_assoc_long(sub, "version_number", (long)MYSQLND_QC_STD_HANDLER_VERSION);
		add_assoc_zval(return_value, MYSQLND_QC_STD_HANDLER_NAME, sub);
	}

	{
		zval * sub;
		MAKE_STD_ZVAL(sub);
		array_init(sub);
		add_assoc_string(sub, "version", estrdup(MYSQLND_QC_USER_HANDLER_VERSION_STR), 0);
		add_assoc_long(sub, "version_number", (long)MYSQLND_QC_USER_HANDLER_VERSION);
		add_assoc_zval(return_value, MYSQLND_QC_USER_HANDLER_NAME, sub);
	}

#ifdef MYSQLND_QC_HAVE_APC
	{
		zval * sub;
		MAKE_STD_ZVAL(sub);
		array_init(sub);
		add_assoc_string(sub, "version", estrdup(MYSQLND_QC_APC_HANDLER_VERSION_STR), 0);
		add_assoc_long(sub, "version_number", (long)MYSQLND_QC_APC_HANDLER_VERSION);
		add_assoc_zval(return_value, MYSQLND_QC_APC_HANDLER_NAME, sub);
	}
#endif

#ifdef MYSQLND_QC_HAVE_MEMCACHE
	{
		zval * sub;
		MAKE_STD_ZVAL(sub);
		array_init(sub);
		add_assoc_string(sub, "version", estrdup(MYSQLND_QC_MEMC_HANDLER_VERSION_STR), 0);
		add_assoc_long(sub, "version_number", (long)MYSQLND_QC_MEMC_HANDLER_VERSION);
		add_assoc_zval(return_value, MYSQLND_QC_MEMC_HANDLER_NAME, sub);
	}
#endif

#ifdef MYSQLND_QC_HAVE_SQLITE
	{
		zval * sub;
		MAKE_STD_ZVAL(sub);
		array_init(sub);
		add_assoc_string(sub, "version", estrdup(MYSQLND_QC_SQLITE_HANDLER_VERSION_STR), 0);
		add_assoc_long(sub, "version_number", (long)MYSQLND_QC_SQLITE_HANDLER_VERSION);
		add_assoc_zval(return_value, MYSQLND_QC_SQLITE_HANDLER_NAME, sub);
	}
#endif

	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ proto bool mysqlnd_qc_clear_cache()
    Flush cache entries */
static PHP_FUNCTION(mysqlnd_qc_clear_cache)
{
	MYSQLND_QC_METHODS * handler = MYSQLND_QC_G(handler);
	enum_func_status ok = FAIL;
	DBG_ENTER("mysqlnd_qc_clear_cache");
	if (zend_parse_parameters_none() == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (handler->clear_cache) {
		ok = handler->clear_cache(TSRMLS_C);
	}

	RETVAL_BOOL((ok == PASS) ? TRUE : FALSE);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto array mysqlnd_qc_get_core_stats()
    Return cache statistics collected by the core of the query cache */
static PHP_FUNCTION(mysqlnd_qc_get_core_stats)
{
	DBG_ENTER("mysqlnd_qc_get_core_stats");
	if (zend_parse_parameters_none() == FAILURE) {
		DBG_VOID_RETURN;
	}

	mysqlnd_fill_stats_hash(mysqlnd_qc_stats, mysqlnd_qc_stats_values_names, return_value TSRMLS_CC ZEND_FILE_LINE_CC);
	mysqlnd_qc_add_to_array_long(return_value, "request_counter", sizeof("request_counter") - 1, MYSQLND_QC_G(request_counter) TSRMLS_CC);
	mysqlnd_qc_add_to_array_long(return_value, "process_hash", sizeof("process_hash") - 1, ((unsigned int)MYSQLND_QC_G(process_hash)) TSRMLS_CC);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto array mysqlnd_qc_get_query_trace_log()
   */
static PHP_FUNCTION(mysqlnd_qc_get_query_trace_log)
{
	DBG_ENTER("zif_mysqlnd_qc_get_query_trace_log");
	if (zend_parse_parameters_none() == FAILURE) {
		DBG_VOID_RETURN;
	}

	mysqlnd_qc_get_query_trace_log(return_value TSRMLS_CC);

	DBG_VOID_RETURN;
}
/* }}} */


#ifdef NORM_QUERY_TRACE_LOG
/* {{{ proto array mysqlnd_qc_get_normalized_query_trace_log()
   */
static PHP_FUNCTION(mysqlnd_qc_get_normalized_query_trace_log)
{
	DBG_ENTER("zif_mysqlnd_qc_get_normalized_query_trace_log");
	if (zend_parse_parameters_none() == FAILURE) {
		DBG_VOID_RETURN;
	}

	mysqlnd_qc_get_normalized_query_trace_log(return_value TSRMLS_CC);

	DBG_VOID_RETURN;
}
/* }}} */
#endif

/* {{{ proto bool mysqlnd_qc_set_cache_condition(int condition_type, mixed condition, mixed condition_option)
   */
static PHP_FUNCTION(mysqlnd_qc_set_cache_condition)
{
	double condition_type;
	zval * condition;
	zval * condition_option;
	DBG_ENTER("zif_mysqlnd_qc_set_cache_condition");

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "dz|z!", &condition_type, &condition, &condition_option) == FAILURE) {
		DBG_VOID_RETURN;
	}

	if (QC_CONDITION_META_SCHEMA_PATTERN == condition_type) {
		/* type, string pattern [, uint ttl] */
		char * pattern;
		uint ttl;
		MYSQLND_QC_CONDITION_PATTERN cond;

		pattern = emalloc(Z_STRLEN_P(condition) + 1);
		memcpy((void *)pattern, (const void*)Z_STRVAL_P(condition), Z_STRLEN_P(condition) + 1);

		convert_to_long(condition_option);
		/* TODO: bail about negative values */
		ttl = (Z_LVAL_P(condition_option) <= 0) ? MYSQLND_QC_G(ttl) : Z_LVAL_P(condition_option);

		cond.pattern = pattern;
		cond.ttl = (uint)ttl;

		if (0 == zend_llist_count(&MYSQLND_QC_G(should_cache_conditions))) {
			MYSQLND_QC_SHOULD_CACHE_CONDITION entry;

			entry.type = QC_CONDITION_META_SCHEMA_PATTERN;
			entry.conditions = mnd_emalloc(sizeof(zend_llist));
			zend_llist_init((zend_llist *)entry.conditions, sizeof(MYSQLND_QC_CONDITION_PATTERN), should_cache_conditions_entry_dtor /*dtor*/, 0 /*non-persistent*/);

			zend_llist_add_element((zend_llist *)entry.conditions, (void*)&cond);
			zend_llist_add_element(&MYSQLND_QC_G(should_cache_conditions), (void*)&entry);

		} else {
			zend_llist_position	pos;
			MYSQLND_QC_SHOULD_CACHE_CONDITION * entry;

			for (entry = (MYSQLND_QC_SHOULD_CACHE_CONDITION *) zend_llist_get_first_ex(&MYSQLND_QC_G(should_cache_conditions), &pos);
					entry;
					entry = (MYSQLND_QC_SHOULD_CACHE_CONDITION *) zend_llist_get_next_ex(&MYSQLND_QC_G(should_cache_conditions), &pos)) {
				/* TODO: search and init logic supports only one kind of entry */
				if (QC_CONDITION_META_SCHEMA_PATTERN == entry->type) {
					if (entry->conditions) {
						zend_llist_add_element((zend_llist *)entry->conditions, (void*)&cond);
					}
				}
			}
		}

		RETVAL_TRUE;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown condition type");
		RETVAL_FALSE;
	}

	DBG_VOID_RETURN;
}
/* }}} */


static uint64_t debug_request_counter = 0;
#ifdef ZTS
	MUTEX_T	LOCK_request_counter_access;
#endif


/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(mysqlnd_qc)
{
	char buf[32];
	struct timeval tp = {0};
	struct timezone tz = {0};

#ifdef ZTS
	LOCK_request_counter_access = tsrm_mutex_alloc();
#endif

	ZEND_INIT_MODULE_GLOBALS(mysqlnd_qc, php_mysqlnd_qc_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	REGISTER_STRING_CONSTANT("MYSQLND_QC_ENABLE_SWITCH",    ENABLE_SWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("MYSQLND_QC_DISABLE_SWITCH",   DISABLE_SWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("MYSQLND_QC_TTL_SWITCH",       ENABLE_SWITCH_TTL, CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("MYSQLND_QC_SERVER_ID_SWITCH", SERVER_ID_SWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("MYSQLND_QC_VERSION", PHP_MYSQLND_QC_VERSION, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("MYSQLND_QC_CONDITION_META_SCHEMA_PATTERN", QC_CONDITION_META_SCHEMA_PATTERN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQLND_QC_VERSION_ID", MYSQLND_QC_VERSION, CONST_CS | CONST_PERSISTENT);

	mysqlnd_qc_plugin_id = mysqlnd_plugin_register();

	mysqlnd_qc_handler_classes_minit(TSRMLS_C);
	mysqlnd_stats_init(&mysqlnd_qc_stats, QC_STAT_LAST);

	if (MYSQLND_QC_G(enable_qc)) {
		mysqlnd_qc_register_hooks();
	}

	/* TODO: Andrey, do we need to init the HT, if QC is disabled? */
#ifdef NORM_QUERY_TRACE_LOG
  #ifdef ZTS
	norm_query_trace_log.LOCK_access = tsrm_mutex_alloc();
  #endif
	zend_hash_init(&norm_query_trace_log.ht, 0, NULL, mysqlnd_qc_norm_query_trace_log_entry_dtor_func, 1 /*persistent*/);
#endif

#ifdef ZTS
	LOCK_qc_methods_access = tsrm_mutex_alloc();
#endif

	{
		uint i;
		for (i = 0; i < sizeof(mysqlnd_qc_handlers) / sizeof(mysqlnd_qc_handlers[0]); i++) {
			if (mysqlnd_qc_handlers[i]->handler_minit) {
				mysqlnd_qc_handlers[i]->handler_minit(TSRMLS_C);
			}
		}
	}

	{
		unsigned int seed = 0;
		size_t buf_len;
		gettimeofday(&tp, &tz);
		seed = (unsigned int)tp.tv_usec * 1000000;
#ifdef PHP_WIN32
		srand(seed);
		buf_len = snprintf(buf, sizeof(buf), "%d", rand());
#else
		buf_len = snprintf(buf, sizeof(buf), "%d", rand_r(&seed));
#endif
		MYSQLND_QC_G(process_hash) = zend_hash_func(buf, buf_len);
	}

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(mysqlnd_qc)
{
	uint i;

	MYSQLND_QC_G(handler) = mysqlnd_qc_methods;
	if (MYSQLND_QC_G(enable_qc)) {
		for (i = 0; i < NUM_USER_HANDLERS; i++) {
			MYSQLND_QC_G(user_handlers_names).names[i] = NULL;
		}
		zend_llist_init(&MYSQLND_QC_G(should_cache_conditions), sizeof(MYSQLND_QC_SHOULD_CACHE_CONDITION), should_cache_conditions_dtor, 0 /*non-persistent*/);
	}
	if (MYSQLND_QC_G(collect_query_trace)) {
		zend_llist_init(&MYSQLND_QC_G(query_trace_log), sizeof(void *), (llist_dtor_func_t) mysqlnd_qc_query_trace_log_entry_dtor_func, 0 /*non-persistent*/);
	}
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(mysqlnd_qc)
{
	DBG_ENTER("RSHUTDOWN");
	MYSQLND_QC_G(request_counter)++;

	if (MYSQLND_QC_G(collect_query_trace)) {
		zend_llist_clean(&MYSQLND_QC_G(query_trace_log));
	}

	if (!MYSQLND_QC_G(enable_qc)) {
		return SUCCESS;
	}

	zend_llist_destroy(&MYSQLND_QC_G(should_cache_conditions));

	if (MYSQLND_QC_G(handler) == &MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_user)) {
		uint i;
		for (i = 0; i < NUM_USER_HANDLERS; i++) {
			if (MYSQLND_QC_G(user_handlers_names).names[i] != NULL) {
				zval_ptr_dtor(&MYSQLND_QC_G(user_handlers_names).names[i]);
				MYSQLND_QC_G(user_handlers_names).names[i] = NULL;
			}
		}
	}

	if (MYSQLND_QC_G(query_is_select)) {
		zval_ptr_dtor(&MYSQLND_QC_G(query_is_select));
	}

	if (MYSQLND_QC_G(handler) == (&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_object)) && MYSQLND_QC_G(handler)->handler_change_shutdown) {
		MYSQLND_QC_G(handler)->handler_change_shutdown(TSRMLS_C);
	}
	MYSQLND_QC_G(handler) = mysqlnd_qc_methods;

	if (MYSQLND_QC_G(collect_statistics) && MYSQLND_QC_G(collect_statistics_log_file)) {
#ifdef ZTS
	  tsrm_mutex_lock(LOCK_request_counter_access);
#endif
	  if ((++debug_request_counter % 10) == 0) {
		zend_bool dbg_skip_trace = FALSE;
		char log_file[256];
		MYSQLND_DEBUG *dbg;
#ifdef ZTS
		tsrm_mutex_unlock(LOCK_request_counter_access);
#endif
		dbg = mysqlnd_debug_init(NULL TSRMLS_CC);
		if (!dbg) {
			return FAILURE;
		}

		snprintf(log_file, sizeof(log_file), "t:a,%s", MYSQLND_QC_G(collect_statistics_log_file));
		dbg->m->set_mode(dbg, log_file);

		if (dbg) {
			zval return_value;
			DBG_INF_FMT_EX(dbg, "-----------------------------");
			DBG_INF_FMT_EX(dbg, "pid=%d", getpid());
			mysqlnd_fill_stats_hash(mysqlnd_qc_stats, mysqlnd_qc_stats_values_names, &return_value TSRMLS_CC ZEND_FILE_LINE_CC);
			{
				zval * values = &return_value;
				zval **values_entry;
				HashPosition pos_values;

				zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(values), &pos_values);
				while (zend_hash_get_current_data_ex(Z_ARRVAL_P(values), (void **)&values_entry, &pos_values) == SUCCESS) {
					ulong	num_key;
					uint	string_key_len;
#if MYSQLND_UNICODE
					zstr	string_key;
					int		s_len;
					char 	*s = NULL;

					TSRMLS_FETCH();
					zend_hash_get_current_key_ex(Z_ARRVAL_P(values), &string_key, &string_key_len, &num_key, 0, &pos_values);

					convert_to_string(*values_entry);

					if (zend_unicode_to_string(ZEND_U_CONVERTER(UG(runtime_encoding_conv)),
											   &s, &s_len, string_key.u, string_key_len TSRMLS_CC) == SUCCESS) {
						DBG_INF_FMT_EX(dbg, "%s=%s", s, Z_STRVAL_PP(values_entry));
					}
					if (s) {
						mnd_efree(s);
					}
#else
					char	*string_key;

					zend_hash_get_current_key_ex(Z_ARRVAL_P(values), &string_key, &string_key_len, &num_key, 0, &pos_values);

					convert_to_string(*values_entry);
					DBG_INF_FMT_EX(dbg, "%s=%s", string_key, Z_STRVAL_PP(values_entry));
#endif
					zend_hash_move_forward_ex(Z_ARRVAL_P(values), &pos_values);
				}
			}
			zval_dtor(&return_value);
			dbg->m->close(dbg);
			dbg->m->free_handle(dbg);
		}
	  } else {
#ifdef ZTS
		tsrm_mutex_unlock(LOCK_request_counter_access);
#endif
	  }
	}


	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(mysqlnd_qc)
{
	uint i;
	for (i = 0; i < sizeof(mysqlnd_qc_handlers) / sizeof(mysqlnd_qc_handlers[0]); i++) {
		if (mysqlnd_qc_handlers[i]->handler_mshutdown) {
			mysqlnd_qc_handlers[i]->handler_mshutdown(TSRMLS_C);
		}
	}

#ifdef ZTS
	tsrm_mutex_free(LOCK_qc_methods_access);
#endif

#ifdef ZTS
	tsrm_mutex_free(LOCK_request_counter_access);
#endif

	mysqlnd_stats_end(mysqlnd_qc_stats);

	mysqlnd_qc_handler_classes_mshutdown(TSRMLS_C);

#ifdef NORM_QUERY_TRACE_LOG
	zend_hash_destroy(&norm_query_trace_log.ht);
  #ifdef ZTS
	tsrm_mutex_free(norm_query_trace_log.LOCK_access);
  #endif
#endif

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(mysqlnd_qc)
{
	char buf[64];
	zval values;
	MYSQLND_QC_METHODS * handler = MYSQLND_QC_G(handler);

	php_info_print_table_start();
	php_info_print_table_header(2, "mysqlnd_qc support", "enabled");

	snprintf(buf, sizeof(buf), "%s (%d)", PHP_MYSQLND_QC_VERSION, MYSQLND_QC_VERSION);
	php_info_print_table_row(2, "Mysqlnd Query Cache (mysqlnd_qc)", buf);
	php_info_print_table_row(2, "enabled", MYSQLND_QC_G(enable_qc) ? "Yes" : "No");
	php_info_print_table_row(2, "Cache by default?", MYSQLND_QC_G(cache_by_default) ? "Yes" : "No");
	php_info_print_table_row(2, "Cache no table?", MYSQLND_QC_G(cache_no_table) ? "Yes" : "No");
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "Handler", "");
	snprintf(buf, sizeof(buf), "%s %s", handler->name, handler->version);
	{
		uint i = 0;
		for (i = 0; i < sizeof(mysqlnd_qc_handlers) / sizeof(mysqlnd_qc_handlers[0]); i++) {
			snprintf(buf, sizeof(buf), "%s", mysqlnd_qc_handlers[i]->name);
			php_info_print_table_row(2, buf, handler == mysqlnd_qc_handlers[i]? "default":"enabled");
		}
	}
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "Statistics", "");
	mysqlnd_fill_stats_hash(mysqlnd_qc_stats, mysqlnd_qc_stats_values_names, &values TSRMLS_CC ZEND_FILE_LINE_CC);
	mysqlnd_minfo_print_hash(&values);
	zval_dtor(&values);

	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_set_is_select, 0, 0, 1)
	ZEND_ARG_INFO(0, is_select)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_set_handlers, 0, 0, NUM_USER_HANDLERS)
	ZEND_ARG_INFO(0, get_hash)
	ZEND_ARG_INFO(0, find_query_in_cache)
	ZEND_ARG_INFO(0, return_to_cache)
	ZEND_ARG_INFO(0, add_query_to_cache_if_not_exists)
	ZEND_ARG_INFO(0, update_query_run_time_stats)
	ZEND_ARG_INFO(0, get_stats)
	ZEND_ARG_INFO(0, clear_cache)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_get_cache_info, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_set_storage_handler, 0, 0, 1)
	ZEND_ARG_INFO(0, handler)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_get_available_handlers, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_clear_cache, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_get_core_stats, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_get_query_trace_log, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_query_is_select, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

#ifdef NORM_QUERY_TRACE_LOG
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_get_normalized_query_trace_log, 0, 0, 0)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_qc_set_cache_condition, 0, 0, 3)
	ZEND_ARG_INFO(0, condition_type)
	ZEND_ARG_INFO(0, condition)
	ZEND_ARG_INFO(0, condition_option)
ZEND_END_ARG_INFO()

static const zend_function_entry mysqlnd_qc_functions[] = {
	PHP_FE(mysqlnd_qc_set_user_handlers,			arginfo_mysqlnd_qc_set_handlers)
	PHP_FE(mysqlnd_qc_get_cache_info,				arginfo_mysqlnd_qc_get_cache_info)
	PHP_FE(mysqlnd_qc_set_is_select,				arginfo_mysqlnd_qc_set_is_select)
	PHP_FE(mysqlnd_qc_set_storage_handler,			arginfo_mysqlnd_qc_set_storage_handler)
	PHP_FE(mysqlnd_qc_get_available_handlers,		arginfo_mysqlnd_qc_get_available_handlers)
	PHP_FE(mysqlnd_qc_clear_cache,					arginfo_mysqlnd_qc_clear_cache)
	PHP_FE(mysqlnd_qc_get_core_stats,				arginfo_mysqlnd_qc_get_core_stats)
	PHP_FE(mysqlnd_qc_get_query_trace_log,			arginfo_mysqlnd_qc_get_query_trace_log)
	PHP_FE(mysqlnd_qc_default_query_is_select,		arginfo_mysqlnd_qc_query_is_select)
	PHP_FE(mysqlnd_qc_nop_query_is_select,			arginfo_mysqlnd_qc_query_is_select)
#ifdef NORM_QUERY_TRACE_LOG
	PHP_FE(mysqlnd_qc_get_normalized_query_trace_log, arginfo_mysqlnd_qc_get_normalized_query_trace_log)
#endif
	PHP_FE(mysqlnd_qc_set_cache_condition,			arginfo_mysqlnd_qc_set_cache_condition)
	{NULL, NULL, NULL}	/* Must be the last line in mysqlnd_qc_functions[] */
};


static const zend_module_dep mysqlnd_qc_deps[] = {

	ZEND_MOD_REQUIRED("mysqlnd")
#ifdef MYSQLND_QC_HAVE_APC
	ZEND_MOD_REQUIRED("apc")
#endif
#ifdef MYSQLND_QC_HAVE_SQLITE
	ZEND_MOD_REQUIRED("sqlite3")
#endif
	{NULL, NULL, NULL}
};


/* {{{ mysqlnd_qc_module_entry
 */
zend_module_entry mysqlnd_qc_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mysqlnd_qc_deps,
	"mysqlnd_qc",
	mysqlnd_qc_functions,
	PHP_MINIT(mysqlnd_qc),
	PHP_MSHUTDOWN(mysqlnd_qc),
	PHP_RINIT(mysqlnd_qc),
	PHP_RSHUTDOWN(mysqlnd_qc),
	PHP_MINFO(mysqlnd_qc),
	PHP_MYSQLND_QC_VERSION,
	PHP_MODULE_GLOBALS(mysqlnd_qc),
	PHP_GINIT(mysqlnd_qc),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
