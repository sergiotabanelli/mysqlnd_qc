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

/* $Id: header 252479 2008-02-07 19:39:50Z iliaa $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_priv.h"
#include "mysqlnd_qc_user_handler.h"

unsigned int mysqlnd_qc_plugin_id;

MYSQLND_STATS * mysqlnd_qc_stats = NULL;

#ifdef ZTS
MUTEX_T LOCK_qc_methods_access;
#endif

#if MYSQLND_VERSION_ID >= 50010
#define QC_LOAD_AND_COPY_CONN_HANDLE_METHODS(orig_methods, qc_methods) \
	(orig_methods) = mysqlnd_conn_get_methods(); \
	memcpy(&(qc_methods), (orig_methods), sizeof(struct st_mysqlnd_conn_methods));

#define QC_CONN_DATA_METHODS() mysqlnd_conn_data_get_methods()

#define QC_SET_CONN_HANDLE_METHODS(qc_methods) mysqlnd_conn_set_methods((qc_methods));

#define QC_LOAD_AND_COPY_CONN_DATA_METHODS(orig_methods, qc_methods) \
	(orig_methods) = mysqlnd_conn_data_get_methods(); \
	memcpy(&(qc_methods), (orig_methods), sizeof(struct st_mysqlnd_conn_data_methods));

#define QC_SET_CONN_DATA_METHODS(qc_methods) mysqlnd_conn_data_set_methods((qc_methods));

#define QC_GET_CONN_DATA_FROM_CONN(conn) (conn)->data

struct st_mysqlnd_conn_data_methods * qc_orig_mysqlnd_conn_methods;
/* static struct st_mysqlnd_conn_methods my_mysqlnd_conn_handle_methods; */

struct st_mysqlnd_conn_methods * qc_orig_mysqlnd_conn_handle_methods;
static struct st_mysqlnd_conn_data_methods my_mysqlnd_conn_methods;

#else

#define QC_LOAD_AND_COPY_CONN_HANDLE_METHODS(orig_methods, qc_methods)

#define QC_CONN_DATA_METHODS() mysqlnd_conn_get_methods()

#define QC_SET_CONN_HANDLE_METHODS(ms_methods)

#define QC_LOAD_AND_COPY_CONN_DATA_METHODS(orig_methods, qc_methods) \
	(orig_methods) = mysqlnd_conn_get_methods(); \
	memcpy(&(qc_methods), (orig_methods), sizeof(struct st_mysqlnd_conn_methods));

#define QC_SET_CONN_DATA_METHODS(qc_methods) mysqlnd_conn_set_methods((qc_methods));

#define QC_GET_CONN_DATA_FROM_CONN(conn) (conn)

struct st_mysqlnd_conn_methods * qc_orig_mysqlnd_conn_methods;
static struct st_mysqlnd_conn_methods my_mysqlnd_conn_methods;
#endif


#if PHP_VERSION_ID < 50400
/* {{{ mysqlnd_qc_receive_record */
enum_func_status
mysqlnd_qc_receive_record(MYSQLND * const conn, zend_uchar * const buffer, size_t count TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_QC_CONNECTION_DATA ** conn_data_pp;
	DBG_ENTER("mysqlnd_qc_receive_record");
	conn_data_pp = (MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_qc_plugin_id);
	/* *conn_data_pp should be always non-NULL or we should not be in this function */
	ret = (*conn_data_pp)->original_receive(conn, (zend_uchar *) buffer, count TSRMLS_CC);
	if (ret == PASS) {
		smart_str_appendl_ex((*conn_data_pp)->recorded_data, buffer, count, PERSISTENT_STR);
		DBG_INF_FMT("appended %u bytes to %u bytes", count, (*conn_data_pp)->recorded_data->len);
		MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_RECEIVE_BYTES_RECORDED, count);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_send_record */
size_t
mysqlnd_qc_send_record(MYSQLND * const conn, char * const buf, size_t count TSRMLS_DC)
{
	MYSQLND_QC_CONNECTION_DATA ** conn_data_pp = (MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_qc_plugin_id);
	size_t ret;
	DBG_ENTER("mysqlnd_qc_send_record");
	/* *conn_data_pp should be always non-NULL or we should not be in this function */
	ret = (*conn_data_pp)->original_send(conn, buf, count TSRMLS_CC);
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_SEND_BYTES_RECORDED, count);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_receive_replay */
enum_func_status
mysqlnd_qc_receive_replay(MYSQLND * conn, zend_uchar * buffer, size_t count TSRMLS_DC)
{
	MYSQLND_QC_CONNECTION_DATA ** conn_data_pp = (MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data(conn, mysqlnd_qc_plugin_id);
	DBG_ENTER("mysqlnd_qc_receive_replay");
	if (((*conn_data_pp)->recorded_data->len - (*conn_data_pp)->recorded_data_current_position) < count) {
		DBG_RETURN(FAIL);
	}
	memcpy(buffer, (*conn_data_pp)->recorded_data->c + (*conn_data_pp)->recorded_data_current_position, count);
	(*conn_data_pp)->recorded_data_current_position += count;
	/* no need to increase packet_no, as on the upper level in mysqlnd_read_header() this will be done */
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_RECEIVE_BYTES_REPLAYED, count);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_qc_send_replay */
size_t
mysqlnd_qc_send_replay(MYSQLND * const conn, char * const buf, size_t count TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_send_replay");
	/* if the packet is MYSQLND_MAX_PACKET_SIZE bytes long there will be a second empty packet */
	conn->net->packet_no+= count / MYSQLND_MAX_PACKET_SIZE + 1;
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_SEND_BYTES_REPLAYED, count);
	DBG_RETURN(count);
}
/* }}} */

#elif PHP_VERSION_ID >= 50400

/* {{{ mysqlnd_qc_receive_record */
enum_func_status
mysqlnd_qc_receive_record(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
						  MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_QC_NET_DATA ** net_data_pp;
	DBG_ENTER("mysqlnd_qc_receive_record");
	net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(net, mysqlnd_qc_plugin_id);
	/* *conn_data_pp should be always non-NULL or we should not be in this function */
	ret = (*net_data_pp)->original_receive(net, buffer, count, conn_stats, error_info TSRMLS_CC);
	if (ret == PASS) {
		smart_str_appendl_ex((*net_data_pp)->recorded_data, buffer, count, PERSISTENT_STR);
		DBG_INF_FMT("appended %u bytes to %u bytes", count, (*net_data_pp)->recorded_data->len);
		MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_RECEIVE_BYTES_RECORDED, count);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_send_record */
size_t
mysqlnd_qc_send_record(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
					   MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC)
{
	MYSQLND_QC_NET_DATA ** net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(net, mysqlnd_qc_plugin_id);
	size_t ret;
	DBG_ENTER("mysqlnd_qc_send_record");
	/* *conn_data_pp should be always non-NULL or we should not be in this function */
	ret = (*net_data_pp)->original_send(net, buffer, count, conn_stats, error_info TSRMLS_CC);
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_SEND_BYTES_RECORDED, count);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_receive_replay */
enum_func_status
mysqlnd_qc_receive_replay(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
						  MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC)
{
	MYSQLND_QC_NET_DATA ** net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(net, mysqlnd_qc_plugin_id);
	DBG_ENTER("mysqlnd_qc_receive_replay");
	if (((*net_data_pp)->recorded_data->len - (*net_data_pp)->recorded_data_current_position) < count) {
		DBG_RETURN(FAIL);
	}
	memcpy(buffer, (*net_data_pp)->recorded_data->c + (*net_data_pp)->recorded_data_current_position, count);
	(*net_data_pp)->recorded_data_current_position += count;
	/* no need to increase packet_no, as on the upper level in mysqlnd_read_header() this will be done */
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_RECEIVE_BYTES_REPLAYED, count);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_qc_send_replay */
size_t
mysqlnd_qc_send_replay(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
					   MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_send_replay");
	/* if the packet is MYSQLND_MAX_PACKET_SIZE bytes long there will be a second empty packet */
	net->packet_no+= count / MYSQLND_MAX_PACKET_SIZE + 1;
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_SEND_BYTES_REPLAYED, count);
	DBG_RETURN(count);
}
/* }}} */

#endif /* PHP_VERSION_ID < 50400 */


/* {{{ mysqlnd_qc::connect */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc, connect)(MYSQLND_CONN_DATA * conn,
									const char * host,
									const char * user,
									const char * passwd,
									unsigned int passwd_len,
									const char * db,
									unsigned int db_len,
									unsigned int port,
									const char * socket_or_pipe,
									unsigned int mysql_flags TSRMLS_DC)
{
	enum_func_status ret;
	QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
	DBG_ENTER("mysqlnd_qc::connect");

	{
		if ((*conn_data_pp) == NULL) {
			DBG_INF("Data is NULL, allocating");
			*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_QC_CONNECTION_DATA), conn->persistent);
			MYSQLND_QC_NET_SET_ORIGINAL(conn, conn_data_pp);
		}
	}
	ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(connect)(conn, host, user, passwd, passwd_len, db, db_len, port,
													 socket_or_pipe, mysql_flags TSRMLS_CC);

	if (PASS == ret) {
		/* it might happen that in connect the memory is released - free_contents called for some reason , then we need to allocate again */
		if ((*conn_data_pp) == NULL) {
			DBG_INF("Data is NULL, allocating");
			*conn_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_QC_CONNECTION_DATA), conn->persistent);
			MYSQLND_QC_NET_SET_ORIGINAL(conn, conn_data_pp);
		}
		(*conn_data_pp)->multi_q_enabled = (mysql_flags & CLIENT_MULTI_STATEMENTS) ? TRUE : FALSE;
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc::set_server_option */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc, set_server_option)(MYSQLND_CONN_DATA * const conn, enum_mysqlnd_server_option option TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_qc::set_server_option");
	ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(set_server_option)(conn, option TSRMLS_CC);
	if (PASS == ret) {
		QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
		if (option == MYSQL_OPTION_MULTI_STATEMENTS_OFF) {
			(*conn_data_pp)->multi_q_enabled = FALSE;
		} else if (option == MYSQL_OPTION_MULTI_STATEMENTS_ON) {
			(*conn_data_pp)->multi_q_enabled = TRUE;
		}
	}
	DBG_RETURN(ret);
}
/* }}} */



#ifdef NORM_QUERY_TRACE_LOG
/* {{{ mysqlnd_qc_add_to_normalized_query_trace_log */
static MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY *
mysqlnd_qc_add_to_normalized_query_trace_log(const char * const query, size_t query_len TSRMLS_DC)
{
	MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY * norm_qt_log_entry = NULL;
	DBG_ENTER("mysqlnd_qc_add_to_normalized_query_trace_log");

	if (MYSQLND_QC_G(collect_normalized_query_trace)) {
		smart_str * normalized_query = mysqlnd_qc_query_tokenize(query, query_len TSRMLS_CC);

		if (normalized_query) {
			MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY ** tmp_norm_qt_log_entry;
			LOCK_QCACHE(norm_query_trace_log);
			/* normalized_query->len includes the \0 byte at the end */
			if (FAILURE == zend_hash_find(&norm_query_trace_log.ht, normalized_query->c, normalized_query->len, (void **) &tmp_norm_qt_log_entry)) {
				norm_qt_log_entry = mnd_calloc(1, sizeof(MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY));
				norm_qt_log_entry->query = mnd_malloc(normalized_query->len); /* -> len includes the \0 */
				norm_qt_log_entry->query_len = normalized_query->len - 1;
				memcpy(norm_qt_log_entry->query, normalized_query->c, normalized_query->len);
				norm_qt_log_entry->eligible_for_caching = FALSE;
	  #ifdef ZTS
				norm_qt_log_entry->LOCK_access = tsrm_mutex_alloc();
	  #endif
				/* normalized_query->len includes the \0 byte at the end */
				zend_hash_add(&norm_query_trace_log.ht, normalized_query->c, normalized_query->len, &norm_qt_log_entry, sizeof(MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY *), NULL);
			} else {
				norm_qt_log_entry = *tmp_norm_qt_log_entry;
			}
			UNLOCK_QCACHE(norm_query_trace_log);
			smart_str_free_ex(normalized_query, 0);
			efree(normalized_query);
		}
	}
	DBG_RETURN(norm_qt_log_entry);
}
/* }}} */


/* {{{ mysqlnd_qc_update_normalized_query_trace_log_entry */
static void
mysqlnd_qc_update_normalized_query_trace_log_entry(MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY * norm_qt_log_entry, uint64_t run_time TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_update_normalized_query_trace_log_entry");
	if (MYSQLND_QC_G(collect_normalized_query_trace) && norm_qt_log_entry) {
		LOCK_QCACHE(*norm_qt_log_entry);
		{
			++norm_qt_log_entry->occurences;
			if (norm_qt_log_entry->min_run_time == 0) {
				norm_qt_log_entry->max_run_time = norm_qt_log_entry->min_run_time = run_time;
			}
			if (run_time > norm_qt_log_entry->max_run_time) {
				norm_qt_log_entry->max_run_time = run_time;
			} else if (run_time < norm_qt_log_entry->min_run_time) {
				norm_qt_log_entry->min_run_time = run_time;
			}
			norm_qt_log_entry->avg_run_time = (norm_qt_log_entry->avg_run_time * (norm_qt_log_entry->occurences - 1)  + run_time) / norm_qt_log_entry->occurences;
		}
		norm_qt_log_entry->eligible_for_caching = TRUE;
		UNLOCK_QCACHE(*norm_qt_log_entry);
	}
	DBG_VOID_RETURN;
}
/* }}} */
#endif


/* {{{ mysqlnd_qc_add_to_query_trace_log */
static MYSQLND_QC_QUERY_TRACE_LOG_ENTRY *
mysqlnd_qc_add_to_query_trace_log(const char * const query, size_t query_len TSRMLS_DC)
{
	MYSQLND_QC_QUERY_TRACE_LOG_ENTRY * qt_log_entry = NULL;
	DBG_ENTER("mysqlnd_qc_add_to_query_trace_log");
	if (MYSQLND_QC_G(collect_query_trace)) {
		qt_log_entry = mnd_ecalloc(1, sizeof(MYSQLND_QC_QUERY_TRACE_LOG_ENTRY));
		qt_log_entry->query = mnd_emalloc(query_len + 1);
		qt_log_entry->query_len = query_len;
		memcpy(qt_log_entry->query, query, query_len);
		qt_log_entry->query[query_len] = '\0';
		qt_log_entry->eligible_for_caching = FALSE;
		qt_log_entry->origin = mysqlnd_get_backtrace(MYSQLND_QC_G(query_trace_bt_depth), &qt_log_entry->origin_len TSRMLS_CC);
		zend_llist_add_element(&MYSQLND_QC_G(query_trace_log), &qt_log_entry);
	}
	DBG_RETURN(qt_log_entry);
}
/* }}} */


/* {{{ mysqlnd_qc_get_last_query_trace_log */
static MYSQLND_QC_QUERY_TRACE_LOG_ENTRY *
mysqlnd_qc_get_last_query_trace_log(TSRMLS_D)
{
	MYSQLND_QC_QUERY_TRACE_LOG_ENTRY * qt_log_entry = NULL, ** qt_log_entry_pp;

	DBG_ENTER("mysqlnd_qc_get_last_query_trace_log");
	if (MYSQLND_QC_G(collect_query_trace)) {
		zend_llist_position * pos = NULL;

		qt_log_entry_pp = (MYSQLND_QC_QUERY_TRACE_LOG_ENTRY **)zend_llist_get_last_ex(&MYSQLND_QC_G(query_trace_log), pos);
		if (qt_log_entry_pp && *qt_log_entry_pp) {
			/* don't copy */
			DBG_RETURN(*qt_log_entry_pp);
		}
	}
	DBG_RETURN(qt_log_entry);
}
/* }}} */


/* {{{ mysqlnd_qc_query_is_cached */
PHP_MYSQLND_QC_API zend_bool
mysqlnd_qc_query_is_cached(MYSQLND_CONN_DATA * conn, const char * const query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC)
{
	zend_bool ret = FALSE;
	MYSQLND_QC_METHODS * handler = MYSQLND_QC_G(handler);

	DBG_ENTER("mysqlnd_qc_is_cached");
	if (handler->query_is_cached) {
		ret = handler->query_is_cached(conn, query, query_len, server_id, server_id_len TSRMLS_CC);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_handle_query */
static enum_func_status
mysqlnd_qc_handle_query(MYSQLND_CONN_DATA * conn, const char * const query, size_t query_len TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	zend_bool try_cache = FALSE;
	char * query_hash_key = NULL;
	size_t query_hash_key_len;
	char * server_id = NULL;
	size_t server_id_len = 0;
	MYSQLND_QC_METHODS * handler = MYSQLND_QC_G(handler);
	MYSQLND_QC_QUERY_TRACE_LOG_ENTRY tmp_qt_log_entry = {0}, * qt_log_entry;
#ifdef NORM_QUERY_TRACE_LOG
	MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY * norm_qt_log_entry = NULL;
#endif
	QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
#if PHP_VERSION_ID >= 50400
	MYSQLND_QC_NET_DATA ** net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(conn->net, mysqlnd_qc_plugin_id);
	MYSQLND_QC_NET_DATA ** recorded_data_pp = net_data_pp;
#elif PHP_VERSION_ID < 50400
	MYSQLND_QC_CONNECTION_DATA ** recorded_data_pp = conn_data_pp;
#endif

	DBG_ENTER("mysqlnd_qc_handle_query");
	DBG_INF_FMT("conn_id=%u", conn->thread_id);

	if (!(qt_log_entry = mysqlnd_qc_add_to_query_trace_log(query, query_len TSRMLS_CC))) {
		qt_log_entry = &tmp_qt_log_entry;
	}

	if ((*conn_data_pp) == NULL) {
		DBG_INF_FMT("conn_data is empty: ret=FAIL");
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Plugin data is empty. If you are trying to use PECL/mysqlnd_ms together with PECL/mysqlnd_qc make sure to compile PECL/mysqlnd_ms appropriately respectively load the PECL/mysqlnd_qc extension into PHP before loading the PECL/mysqlnd_ms extension into PHP to avoid issues with the plugin initialization order", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(ret);
	}
#ifdef NORM_QUERY_TRACE_LOG
	if ((*conn_data_pp)->multi_q_enabled == FALSE) {
		norm_qt_log_entry = mysqlnd_qc_add_to_normalized_query_trace_log(query, query_len TSRMLS_CC);
	}
#endif

	(*conn_data_pp)->should_cache = mysqlnd_qc_query_is_select(query, query_len, &(*conn_data_pp)->ttl, &server_id, &server_id_len TSRMLS_CC);

	DBG_INF_FMT("should_cache=%u", (*conn_data_pp)->should_cache);
	MYSQLND_QC_INC_STATISTIC((*conn_data_pp)->should_cache? QC_STAT_QUERY_SHOULD_CACHE : QC_STAT_QUERY_SHOULD_NOT_CACHE);

	/* TODO: Andrey, we need to discuss this
	 *
	 * You once proposed we can do schema based caching by analyzing result set meta.
	 * I hacked mysqlnd_qc_set_cache_condition() to give it a try. If cache conditions
	 * are set the final decision whether to cache or not is delayed until we have
	 * analyzed result set meta. To be able to do it, we must do some kind of
	 * optimistic caching. That's what try_cache is about. Hacked for unbuffered only.
	 *
	 * The implementation ignores the disable SQL hint
	 */
	try_cache = (zend_llist_count(&MYSQLND_QC_G(should_cache_conditions)) > 0) ? TRUE : FALSE;

	if (((FALSE == (*conn_data_pp)->should_cache) && (FALSE == try_cache)) ||
		!(query_hash_key = handler->get_hash_key(conn, query, query_len, &query_hash_key_len, server_id, server_id_len, conn->persistent TSRMLS_CC)))
	{
		STATS_TIME_SET2((*conn_data_pp)->run_time, (*conn_data_pp)->prev_start_time);
			ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(send_query)(conn, query, query_len TSRMLS_CC);
		STATS_TIME_DIFF((*conn_data_pp)->run_time);
		(*conn_data_pp)->query_active_with_cache = FALSE;
		MYSQLND_QC_INC_STATISTIC(QC_STAT_QUERY_NOT_CACHED);
		if (server_id) {
			efree(server_id);
		}
		DBG_RETURN(ret);
	}

	if (server_id) {
		efree(server_id);
	}
	qt_log_entry->eligible_for_caching = TRUE;

	DBG_INF("Cache this query");
	(*conn_data_pp)->query_active_with_cache = TRUE;
	{
		smart_str * cached_query = handler->find_query_in_cache(query_hash_key, query_hash_key_len TSRMLS_CC);
		if (cached_query) {
			DBG_INF("Found it in the cache");
			MYSQLND_QC_INC_STATISTIC(QC_STAT_QUERY_FOUND_IN_CACHE);
			MYSQLND_QC_NET_SET_REPLAY(conn);
			(*recorded_data_pp)->recorded_data = cached_query;
			(*recorded_data_pp)->recorded_data_current_position = (size_t)0;
		} else {
			DBG_INF("Not in the cache");
#if PHP_VERSION_ID < 50400
			if (conn->net->m.receive == mysqlnd_qc_receive_record) {
#elif PHP_VERSION_ID >= 50400 && PHP_VERSION_ID < 50500
			if (conn->net->m.receive_ex == mysqlnd_qc_receive_record) {
#elif PHP_VERSION_ID >= 50500
			if (conn->net->data->m.receive_ex == mysqlnd_qc_receive_record) {
#endif
				smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
				memset((*recorded_data_pp)->recorded_data, 0, sizeof(smart_str));
			} else {
				(*recorded_data_pp)->recorded_data = mnd_calloc(1, sizeof(smart_str));
				DBG_INF_FMT("new recorded_data %p", (*recorded_data_pp)->recorded_data);
				MYSQLND_QC_NET_SET_RECORD(conn);
			}
		}
	}

	/* ******************** */
	STATS_TIME_SET2((*conn_data_pp)->run_time, (*conn_data_pp)->prev_start_time);
		ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(send_query)(conn, query, query_len TSRMLS_CC);
	STATS_TIME_DIFF((*conn_data_pp)->run_time);
	/* ******************** */

	if (PASS == ret) {
		if ((*conn_data_pp)->query_hash_key) {
			/* don't use mnd_ because the hash_key wasn't allocated with mnd_ and should not be, in some cases data comes from userland */
			pefree((*conn_data_pp)->query_hash_key, conn->persistent);
			(*conn_data_pp)->query_hash_key = NULL;
		}
		COPY_HASH_KEY(query_hash_key, query_hash_key_len, (*conn_data_pp)->query_hash_key, (*conn_data_pp)->query_hash_key_len);
#ifdef NORM_QUERY_TRACE_LOG
		(*conn_data_pp)->norm_qt_log_entry = norm_qt_log_entry;
#endif
	} else {
		DBG_INF_FMT("send_query failed");
		qt_log_entry->run_time = (*conn_data_pp)->run_time;
		MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_QUERY_AGGR_RUN_TIME_TOTAL, (*conn_data_pp)->run_time);
		MYSQLND_QC_INC_STATISTIC(QC_STAT_QUERY_UNCACHED_OTHER);
		FREE_SMART_STR_AND_CONTAINER_AND_NULL((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
		/* don't use mnd_ because the hash_key wasn't allocated with mnd_ and should not be, in some cases data comes from userland */
		pefree(query_hash_key, conn->persistent);
		MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, conn_data_pp);
#ifdef NORM_QUERY_TRACE_LOG
		(*conn_data_pp)->norm_qt_log_entry = NULL; /* it is in the hash and it will clean it */
#endif
	}


	DBG_RETURN(ret);
}
/* }}} */

/* {{{ mysqlnd_qc::reap_query */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc, reap_query)(MYSQLND_CONN_DATA * conn TSRMLS_DC)
{
	enum_func_status ret;
	MYSQLND_QC_QUERY_TRACE_LOG_ENTRY * qt_log_entry;
	QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
#if PHP_VERSION_ID >= 50400
	MYSQLND_QC_NET_DATA ** net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(conn->net, mysqlnd_qc_plugin_id);
	MYSQLND_QC_NET_DATA ** recorded_data_pp = net_data_pp;
#elif PHP_VERSION_ID < 50400
	MYSQLND_QC_CONNECTION_DATA ** recorded_data_pp = conn_data_pp;
#endif
	uint64_t reap_time;

	DBG_ENTER("mysqlnd_qc:reap_query");
	DBG_INF_FMT("conn_id=%u", conn->thread_id);

	STATS_TIME_SET(reap_time);
	ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(reap_query)(conn TSRMLS_CC);
	STATS_TIME_DIFF2(reap_time, (*conn_data_pp)->prev_start_time);
	(*conn_data_pp)->run_time += reap_time;

	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_QUERY_AGGR_RUN_TIME_TOTAL, (*conn_data_pp)->prev_start_time);

	if ((qt_log_entry = mysqlnd_qc_get_last_query_trace_log(TSRMLS_C))) {
		qt_log_entry->run_time = (*conn_data_pp)->prev_start_time;
	}

#ifdef NORM_QUERY_TRACE_LOG
	if ((*conn_data_pp)->norm_qt_log_entry) {
		mysqlnd_qc_update_normalized_query_trace_log_entry((*conn_data_pp)->norm_qt_log_entry, (*conn_data_pp)->prev_start_time TSRMLS_CC);
	}
#endif

	if (TRUE == (*conn_data_pp)->query_active_with_cache) {
		if (ret == PASS && QC_CONN_DATA_METHODS()->get_field_count(conn TSRMLS_CC)) {
			MYSQLND_QC_INC_STATISTIC(QC_STAT_QUERY_COULD_CACHE);
			DBG_INF("Resultset query");
		} else {
			DBG_INF_FMT("Not a resultset query or error. Restoring original stream_read handler. ret=%d", ret);
			MYSQLND_QC_INC_STATISTIC(QC_STAT_QUERY_UNCACHED_OTHER);
			FREE_SMART_STR_AND_CONTAINER_AND_NULL((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
			/* don't use mnd_ because the hash_key wasn't allocated with mnd_ and should not be, in some cases data comes from userland */
			MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, conn_data_pp);
#ifdef NORM_QUERY_TRACE_LOG
			(*conn_data_pp)->norm_qt_log_entry = NULL; /* it is in the hash and it will clean it */
		}
#endif
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_qc, send_query) */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc, send_query)(MYSQLND_CONN_DATA * conn, const char *query, unsigned int query_len TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc::send_query");
	DBG_RETURN(mysqlnd_qc_handle_query(conn, query, query_len TSRMLS_CC));
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_qc, store_result) */
static MYSQLND_RES *
#if PHP_VERSION_ID < 50600
MYSQLND_METHOD(mysqlnd_qc, store_result)(MYSQLND_CONN_DATA * const conn TSRMLS_DC)
#else
MYSQLND_METHOD(mysqlnd_qc, store_result)(MYSQLND_CONN_DATA * const conn, const unsigned int flags TSRMLS_DC)
#endif
{
	MYSQLND_RES * result;
	uint64_t store_time;
	MYSQLND_QC_METHODS * handler = MYSQLND_QC_G(handler);
	DBG_ENTER("mysqlnd_qc::store_result");

	STATS_TIME_SET(store_time);
#if PHP_VERSION_ID < 50600
		result = QC_CALL_ORIGINAL_CONN_DATA_METHOD(store_result)(conn TSRMLS_CC);
#else
		result = QC_CALL_ORIGINAL_CONN_DATA_METHOD(store_result)(conn, flags TSRMLS_CC);
#endif
	STATS_TIME_DIFF(store_time);
	MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_QUERY_AGGR_STORE_TIME_TOTAL, (uint64_t)store_time);

	if (result) {
		MYSQLND_QC_QUERY_TRACE_LOG_ENTRY tmp_on_stack_entry = {0}, * last_qt_log_entry = &tmp_on_stack_entry;
#ifdef NORM_QUERY_TRACE_LOG
		MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY tmp_norm_qt_log_entry = {0}, *norm_qt_log_entry = &tmp_norm_qt_log_entry;
#endif
		QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
#if PHP_VERSION_ID >= 50400
		MYSQLND_QC_NET_DATA ** net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(conn->net, mysqlnd_qc_plugin_id);
		MYSQLND_QC_NET_DATA ** recorded_data_pp = net_data_pp;
#elif PHP_VERSION_ID < 50400
		MYSQLND_QC_CONNECTION_DATA ** recorded_data_pp = conn_data_pp;
#endif
		DBG_INF("Store result was successful");

		if (MYSQLND_QC_G(collect_query_trace)) {
			last_qt_log_entry = *(MYSQLND_QC_QUERY_TRACE_LOG_ENTRY **) zend_llist_get_last(&MYSQLND_QC_G(query_trace_log));
			last_qt_log_entry->store_time = store_time;
		}

#ifdef NORM_QUERY_TRACE_LOG
		if (MYSQLND_QC_G(collect_normalized_query_trace) && (*conn_data_pp)->norm_qt_log_entry) {
			norm_qt_log_entry = (*conn_data_pp)->norm_qt_log_entry;
			LOCK_QCACHE(*norm_qt_log_entry);
			if (norm_qt_log_entry->min_store_time == 0) {
				norm_qt_log_entry->max_store_time = norm_qt_log_entry->min_store_time = store_time;
			}
			if (store_time > norm_qt_log_entry->max_store_time) {
				norm_qt_log_entry->max_store_time = store_time;
			} else if (store_time < norm_qt_log_entry->min_store_time) {
				norm_qt_log_entry->min_store_time = store_time;
			}
			norm_qt_log_entry->avg_store_time = (norm_qt_log_entry->avg_store_time * (norm_qt_log_entry->occurences - 1)  + store_time) / norm_qt_log_entry->occurences;
			UNLOCK_QCACHE(*norm_qt_log_entry);
		}
#endif

		if ((*conn_data_pp) != NULL && (*conn_data_pp)->query_active_with_cache) {
			DBG_INF("Data is not NULL");
#if PHP_VERSION_ID < 50400
			if (conn->net->m.receive == mysqlnd_qc_receive_replay) {
#elif PHP_VERSION_ID >= 50400 && PHP_VERSION_ID < 50500
			if (conn->net->m.receive_ex == mysqlnd_qc_receive_replay) {
#elif PHP_VERSION_ID >= 50500
			if (conn->net->data->m.receive_ex == mysqlnd_qc_receive_replay) {
#endif
				last_qt_log_entry->was_already_in_cache = TRUE;
				MYSQLND_QC_INC_STATISTIC(QC_STAT_HIT);
				MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_QUERY_AGGR_STORE_TIME_CACHE_HIT, (uint64_t)store_time);
				MYSQLND_QC_INC_STATISTIC_W_VALUE(QC_STAT_QUERY_AGGR_RUN_TIME_CACHE_HIT, (uint64_t)(*conn_data_pp)->run_time);
				DBG_INF("Swichting from read_replay to original_receive");
				handler->return_to_cache((*conn_data_pp)->query_hash_key,
										(*conn_data_pp)->query_hash_key_len,
										(*recorded_data_pp)->recorded_data
										TSRMLS_CC);

				handler->update_query_run_time_stats((*conn_data_pp)->query_hash_key,
													(*conn_data_pp)->query_hash_key_len,
													(*conn_data_pp)->run_time,
													store_time
													TSRMLS_CC);
#if PHP_VERSION_ID < 50400
			} else if (conn->net->m.receive == mysqlnd_qc_receive_record) {
#elif PHP_VERSION_ID >= 50400 && PHP_VERSION_ID < 50500
			} else if (conn->net->m.receive_ex == mysqlnd_qc_receive_record) {
#elif PHP_VERSION_ID >= 50500
			} else if (conn->net->data->m.receive_ex == mysqlnd_qc_receive_record) {
#endif
				zend_bool no_table = FALSE;
				DBG_INF("Cache used stream_read_record");
				if (!MYSQLND_QC_G(cache_no_table)) {
					unsigned int i;
					for (i = 0; (i < mysqlnd_num_fields(result)) && !no_table; i++) {
						const MYSQLND_FIELD *field = mysqlnd_fetch_field_direct(result, i);
						no_table = (field->table_length == 0) ? TRUE : FALSE;
					}
				}

				if (TRUE == no_table) {
					last_qt_log_entry->no_table = TRUE;
					MYSQLND_QC_INC_STATISTIC_W_VALUE2(QC_STAT_MISS, 1, QC_STAT_QUERY_UNCACHED_NO_TABLE, 1);
					DBG_INF("Don't add to cache - no table name for at least one column");
					smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
					mnd_free((*recorded_data_pp)->recorded_data);
				} else {
					uint ttl = 0;

					/* Either is_select() allowed caching (should_cache = true) or this is optimistic mode and we need to check result set meta */
					if ((TRUE == (*conn_data_pp)->should_cache) ||
						(TRUE == mysqlnd_qc_user_should_cache(result,
																(*conn_data_pp)->query_hash_key,
																(*conn_data_pp)->query_hash_key_len,
																(*recorded_data_pp)->recorded_data,
																&ttl,
																(*conn_data_pp)->run_time,
																store_time,
																result->stored_data->row_count
																TSRMLS_CC))) {
						DBG_INF("Should cache allows caching");
						if ((*conn_data_pp)->ttl) {
							/* SQL hint */
							ttl = (*conn_data_pp)->ttl;
						} else {
							/* Default or condition */
							ttl = (ttl) ? ttl : MYSQLND_QC_G(ttl);
						}
						if (PASS == handler->add_query_to_cache_if_not_exists(result,
																				(*conn_data_pp)->query_hash_key,
																				(*conn_data_pp)->query_hash_key_len,
																				(*recorded_data_pp)->recorded_data,
																				ttl,
																				(*conn_data_pp)->run_time,
																				store_time,
																				result->stored_data->row_count
																				TSRMLS_CC))
						{
							last_qt_log_entry->was_added = TRUE;
							MYSQLND_QC_INC_STATISTIC_W_VALUE2(QC_STAT_MISS, 1, QC_STAT_PUT, 1);
							MYSQLND_QC_INC_STATISTIC_W_VALUE2(QC_STAT_QUERY_AGGR_STORE_TIME_CACHE_PUT, (uint64_t)store_time, QC_STAT_QUERY_AGGR_RUN_TIME_CACHE_PUT, (uint64_t)(*conn_data_pp)->run_time);
							DBG_INF("Added to the cache");
						} else {
							last_qt_log_entry->was_already_in_cache = TRUE;
							MYSQLND_QC_INC_STATISTIC(QC_STAT_HIT);
							MYSQLND_QC_INC_STATISTIC_W_VALUE3(QC_STAT_HIT, 1, QC_STAT_QUERY_AGGR_STORE_TIME_CACHE_HIT,
													  (uint64_t)store_time, QC_STAT_QUERY_AGGR_RUN_TIME_CACHE_HIT,
													   (uint64_t)(*conn_data_pp)->run_time);
							DBG_INF("Query already cached");
							smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
							mnd_free((*recorded_data_pp)->recorded_data);
						}
					} else {
						DBG_INF("Should cache has rejected query");
					}
				}
			}
			(*recorded_data_pp)->recorded_data = NULL;
			MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, conn_data_pp);

			DBG_INF("Freeing hash key");
			/* don't use mnd_ because the hash_key wasn't allocated with mnd_ and should not be, in some cases data comes from userland */
			pefree((*conn_data_pp)->query_hash_key, conn->persistent);
			(*conn_data_pp)->query_hash_key = NULL;
		}
	} else {
		MYSQLND_QC_INC_STATISTIC_W_VALUE2(QC_STAT_MISS, 1, QC_STAT_QUERY_UNCACHED_NO_RESULT, 1);
	}
	DBG_RETURN(result);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_qc, use_result) */
static MYSQLND_RES *
#if PHP_VERSION_ID < 50600
MYSQLND_METHOD(mysqlnd_qc, use_result)(MYSQLND_CONN_DATA * const conn TSRMLS_DC)
#else
MYSQLND_METHOD(mysqlnd_qc, use_result)(MYSQLND_CONN_DATA * const conn, const unsigned int flags TSRMLS_DC)
#endif
{
	MYSQLND_RES * ret;
	DBG_ENTER("mysqlnd_qc::use_result");
	MYSQLND_QC_INC_STATISTIC_W_VALUE2(QC_STAT_MISS, 1, QC_STAT_QUERY_UNCACHED_USE_RESULT, 1);

#if PHP_VERSION_ID < 50600
	ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(use_result)(conn TSRMLS_CC);
#else
	ret = QC_CALL_ORIGINAL_CONN_DATA_METHOD(use_result)(conn, flags TSRMLS_CC);
#endif
	if (ret) {
		QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);

		if ((*conn_data_pp) != NULL) {
#if PHP_VERSION_ID >= 50400
			MYSQLND_QC_NET_DATA ** net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(conn->net, mysqlnd_qc_plugin_id);
			MYSQLND_QC_NET_DATA ** recorded_data_pp = net_data_pp;
#elif PHP_VERSION_ID < 50400
			MYSQLND_QC_CONNECTION_DATA ** recorded_data_pp = conn_data_pp;
#endif

			DBG_INF("Data is not NULL");
			/* don't use mnd_ because the hash_key wasn't allocated with mnd_ and should not be, in some cases data comes from userland */
			if ((*conn_data_pp)->query_hash_key) {
				pefree((*conn_data_pp)->query_hash_key, conn->persistent);
			}
			(*conn_data_pp)->query_hash_key = NULL;

#if PHP_VERSION_ID < 50400
			if (conn->net->m.receive == mysqlnd_qc_receive_replay) {
#elif PHP_VERSION_ID >= 50400 && PHP_VERSION_ID < 50500
			if (conn->net->m.receive_ex == mysqlnd_qc_receive_replay) {
#elif PHP_VERSION_ID >= 50500
			if (conn->net->data->m.receive_ex == mysqlnd_qc_receive_replay) {
#endif
				MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, conn_data_pp);
				(*recorded_data_pp)->recorded_data = NULL;
#if PHP_VERSION_ID < 50400
			} else if (conn->net->m.receive == mysqlnd_qc_receive_record) {
#elif PHP_VERSION_ID >= 50400 && PHP_VERSION_ID < 50500
			} else if (conn->net->m.receive_ex == mysqlnd_qc_receive_record) {
#elif PHP_VERSION_ID >= 50500
			} else if (conn->net->data->m.receive_ex == mysqlnd_qc_receive_record) {
#endif
				MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, conn_data_pp);
				smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
				mnd_free((*recorded_data_pp)->recorded_data);
				(*recorded_data_pp)->recorded_data = NULL;
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ MYSQLND_METHOD(mysqlnd_qc, free_contents) */
static void
MYSQLND_METHOD(mysqlnd_qc, free_contents)(MYSQLND_CONN_DATA * conn TSRMLS_DC)
{
	QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
#if PHP_VERSION_ID >= 50400
	QC_DECLARE_AND_LOAD_NET_DATA(net_data_pp, conn);
#endif
	DBG_ENTER("mysqlnd_qc::free_contents");

	DBG_INF_FMT("conn_data_pp=%p", conn_data_pp);
	if (conn_data_pp && *conn_data_pp) {
		if ((*conn_data_pp)->query_hash_key) {
			DBG_INF_FMT("Freeing query hash key %p", (*conn_data_pp)->query_hash_key);
			/* don't use mnd_ because the hash_key wasn't allocated with mnd_ and should not be, in some cases data comes from userland */
			pefree((*conn_data_pp)->query_hash_key, conn->persistent);
			(*conn_data_pp)->query_hash_key = NULL;
		}
		DBG_INF("Destroying hash");
		mnd_pefree(*conn_data_pp, conn->persistent);
		*conn_data_pp = NULL;
	}
#if PHP_VERSION_ID >= 50400
	if (net_data_pp && *net_data_pp) {
		mnd_pefree(*net_data_pp, conn->persistent);
		*net_data_pp = NULL;
	}
#endif
	QC_CALL_ORIGINAL_CONN_DATA_METHOD(free_contents)(conn TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_register_hooks */
void
mysqlnd_qc_register_hooks()
{
	QC_LOAD_AND_COPY_CONN_DATA_METHODS(qc_orig_mysqlnd_conn_methods, my_mysqlnd_conn_methods);

	my_mysqlnd_conn_methods.connect	= MYSQLND_METHOD(mysqlnd_qc, connect);

	my_mysqlnd_conn_methods.set_server_option = MYSQLND_METHOD(mysqlnd_qc, set_server_option);

	my_mysqlnd_conn_methods.send_query = MYSQLND_METHOD(mysqlnd_qc, send_query);

	my_mysqlnd_conn_methods.reap_query = MYSQLND_METHOD(mysqlnd_qc, reap_query);

	my_mysqlnd_conn_methods.store_result = MYSQLND_METHOD(mysqlnd_qc, store_result);

	my_mysqlnd_conn_methods.use_result = MYSQLND_METHOD(mysqlnd_qc, use_result);

	my_mysqlnd_conn_methods.free_contents = MYSQLND_METHOD(mysqlnd_qc, free_contents);

	QC_SET_CONN_DATA_METHODS(&my_mysqlnd_conn_methods);

	mysqlnd_qc_ps_register_hooks();
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
