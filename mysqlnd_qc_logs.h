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

#ifndef MYSQLND_QC_LOGS_H
#define MYSQLND_QC_LOGS_H

#define NORM_QUERY_TRACE_LOG 1

#ifdef NORM_QUERY_TRACE_LOG
typedef struct st_mysqlnd_qc_norm_query_trace_log_entry {
	char * query;
	size_t query_len;
	uint64_t occurences;
	uint64_t min_run_time;
	uint64_t avg_run_time;
	uint64_t max_run_time;
	uint64_t min_store_time;
	uint64_t avg_store_time;
	uint64_t max_store_time;
	zend_bool eligible_for_caching;
#ifdef ZTS
	MUTEX_T		LOCK_access;
#endif
} MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY;
#endif

typedef struct st_mysqlnd_qc_query_trace_log_entry {
	char * query;
	size_t query_len;
	char * origin;
	size_t origin_len;
	uint64_t run_time;
	uint64_t store_time;
	zend_bool eligible_for_caching;
	zend_bool no_table;
	zend_bool was_added;
	zend_bool was_already_in_cache;
} MYSQLND_QC_QUERY_TRACE_LOG_ENTRY;

#ifdef NORM_QUERY_TRACE_LOG
extern struct st_mysqlnd_qc_qcache norm_query_trace_log;
#endif

void mysqlnd_qc_query_trace_log_entry_dtor_func(void * pDest);
void mysqlnd_qc_norm_query_trace_log_entry_dtor_func(void * pDest);
PHP_MYSQLND_QC_API void mysqlnd_qc_get_query_trace_log(zval * return_value TSRMLS_DC);
PHP_MYSQLND_QC_API void mysqlnd_qc_get_normalized_query_trace_log(zval * return_value TSRMLS_DC);


#endif /* MYSQLND_QC_LOGS_H */
