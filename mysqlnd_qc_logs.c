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
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_logs.h"

/* {{{ mysqlnd_qc_query_trace_log_entry_dtor_func */
void
mysqlnd_qc_query_trace_log_entry_dtor_func(void * pDest)
{
	MYSQLND_QC_QUERY_TRACE_LOG_ENTRY * entry = *(MYSQLND_QC_QUERY_TRACE_LOG_ENTRY **) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_qc_query_trace_log_entry_dtor_func");
	if (entry->query) {
		mnd_efree(entry->query);
		entry->query = NULL;
	}
	if (entry->origin) {
		efree(entry->origin);
		entry->origin = NULL;
	}
	mnd_efree(entry);
	DBG_VOID_RETURN;
}
/* }}} */


#ifdef NORM_QUERY_TRACE_LOG
/* {{{ mysqlnd_qc_norm_query_trace_log_entry_dtor_func */
void
mysqlnd_qc_norm_query_trace_log_entry_dtor_func(void * pDest)
{
	MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY * entry = *(MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY **) pDest;
	TSRMLS_FETCH();
	DBG_ENTER("mysqlnd_qc_norm_query_trace_log_entry_dtor_func");
	if (entry->query) {
		mnd_free(entry->query);
		entry->query = NULL;
	}
#ifdef ZTS
	tsrm_mutex_free(entry->LOCK_access);
#endif
	mnd_free(entry);
	DBG_VOID_RETURN;
}
/* }}} */
#endif


/* {{{ mysqlnd_qc_get_query_trace_log */
PHP_MYSQLND_QC_API void
mysqlnd_qc_get_query_trace_log(zval * return_value TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_get_query_trace_log");
	array_init(return_value);
	if (MYSQLND_QC_G(collect_query_trace)) {
		MYSQLND_QC_QUERY_TRACE_LOG_ENTRY ** element;

		for (element = zend_llist_get_first(&MYSQLND_QC_G(query_trace_log)); element; element = zend_llist_get_next(&MYSQLND_QC_G(query_trace_log))) {
			zval * query_call;
			MAKE_STD_ZVAL(query_call);
			array_init(query_call);
			add_assoc_stringl_ex(query_call, "query", sizeof("query"), (*element)->query, (*element)->query_len, 1);
			add_assoc_stringl_ex(query_call, "origin", sizeof("origin"), (*element)->origin, (*element)->origin_len, 1);
			add_assoc_long_ex(query_call, "run_time", sizeof("run_time"), (*element)->run_time);
			add_assoc_long_ex(query_call, "store_time", sizeof("store_time"), (*element)->store_time);
			add_assoc_bool_ex(query_call, "eligible_for_caching", sizeof("eligible_for_caching"), (*element)->eligible_for_caching);
			add_assoc_bool_ex(query_call, "no_table", sizeof("no_table"), (*element)->no_table);
			add_assoc_bool_ex(query_call, "was_added", sizeof("was_added"), (*element)->was_added);
			add_assoc_bool_ex(query_call, "was_already_in_cache", sizeof("was_already_in_cache"), (*element)->was_already_in_cache);

			add_next_index_zval(return_value, query_call);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


#ifdef NORM_QUERY_TRACE_LOG
/* {{{ mysqlnd_qc_get_normalized_query_trace_log */
PHP_MYSQLND_QC_API void
mysqlnd_qc_get_normalized_query_trace_log(zval * return_value TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_get_normalized_query_trace_log");
	array_init(return_value);
	if (MYSQLND_QC_G(collect_normalized_query_trace)) {
		MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY **values_entry;
		HashPosition pos_values;

		LOCK_QCACHE(norm_query_trace_log);

		zend_hash_internal_pointer_reset_ex(&norm_query_trace_log.ht, &pos_values);
		while (zend_hash_get_current_data_ex(&norm_query_trace_log.ht, (void **)&values_entry, &pos_values) == SUCCESS) {
			zval * query_call;
			MAKE_STD_ZVAL(query_call);
			array_init(query_call);

			add_assoc_stringl_ex(query_call, "query", sizeof("query"), (*values_entry)->query, (*values_entry)->query_len, 1);

			/* the adding of the query doesn't need protection because it never changes, the other values change, however */
			LOCK_QCACHE(**values_entry);
			add_assoc_long_ex(query_call, "occurences", sizeof("occurences"), (*values_entry)->occurences);
			add_assoc_bool_ex(query_call, "eligible_for_caching", sizeof("eligible_for_caching"), (*values_entry)->eligible_for_caching);
			add_assoc_long_ex(query_call, "avg_run_time", sizeof("avg_run_time"), (*values_entry)->avg_run_time);
			add_assoc_long_ex(query_call, "min_run_time", sizeof("min_run_time"), (*values_entry)->min_run_time);
			add_assoc_long_ex(query_call, "max_run_time", sizeof("max_run_time"), (*values_entry)->max_run_time);
			add_assoc_long_ex(query_call, "avg_store_time", sizeof("avg_store_time"), (*values_entry)->avg_store_time);
			add_assoc_long_ex(query_call, "min_store_time", sizeof("min_store_time"), (*values_entry)->min_store_time);
			add_assoc_long_ex(query_call, "max_store_time", sizeof("max_store_time"), (*values_entry)->max_store_time);
			UNLOCK_QCACHE(**values_entry);

			add_next_index_zval(return_value, query_call);

			zend_hash_move_forward_ex(&norm_query_trace_log.ht, &pos_values);
		}
		UNLOCK_QCACHE(norm_query_trace_log);
	}
	DBG_VOID_RETURN;
}
/* }}} */
#endif
