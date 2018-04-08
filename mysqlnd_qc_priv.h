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

#ifndef MYSQLND_QC_PRIV_H
#define MYSQLND_QC_PRIV_H

#include "mysqlnd_qc_logs.h"


#if MYSQLND_VERSION_ID >= 50010
#define QC_CONN_DATA(conn) ((conn)->data)

#define QC_DECLARE_AND_LOAD_CONN_DATA(conn_data, connection) \
	MYSQLND_QC_CONNECTION_DATA ** conn_data = \
		(MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data_data((connection), mysqlnd_qc_plugin_id)

#define QC_LOAD_CONN_DATA(conn_data, connection) \
	(conn_data) = (MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data_data((connection), mysqlnd_qc_plugin_id)

#define QC_CALL_ORIGINAL_CONN_HANDLE_METHOD(method) qc_orig_mysqlnd_conn_handle_methods->method
#define QC_CALL_ORIGINAL_CONN_DATA_METHOD(method) qc_orig_mysqlnd_conn_methods->method
extern struct st_mysqlnd_conn_data_methods * qc_orig_mysqlnd_conn_methods;
extern struct st_mysqlnd_conn_methods * qc_orig_mysqlnd_conn_handle_methods;

#else

#define QC_CONN_DATA(conn) (conn)

#define QC_DECLARE_AND_LOAD_CONN_DATA(conn_data, connection) \
	MYSQLND_QC_CONNECTION_DATA ** conn_data = \
		(MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data((connection), mysqlnd_qc_plugin_id)

#define QC_LOAD_CONN_DATA(conn_data, connection) \
	(conn_data) = (MYSQLND_QC_CONNECTION_DATA **) mysqlnd_plugin_get_plugin_connection_data((connection), mysqlnd_qc_plugin_id)

#define QC_CALL_ORIGINAL_CONN_HANDLE_METHOD(method) qc_orig_mysqlnd_conn_methods->method
#define QC_CALL_ORIGINAL_CONN_DATA_METHOD(method) qc_orig_mysqlnd_conn_methods->method
extern struct st_mysqlnd_conn_methods * qc_orig_mysqlnd_conn_methods;
#endif


extern unsigned int mysqlnd_qc_plugin_id;

void mysqlnd_qc_ps_register_hooks();

#if PHP_VERSION_ID < 50400
enum_func_status mysqlnd_qc_receive_record(MYSQLND * const conn, zend_uchar * const buf, size_t count TSRMLS_DC);
enum_func_status mysqlnd_qc_receive_replay(MYSQLND * conn, zend_uchar * buffer, size_t count TSRMLS_DC);
size_t mysqlnd_qc_send_record(MYSQLND * const conn, char * const buf, size_t count TSRMLS_DC);
size_t mysqlnd_qc_send_replay(MYSQLND * const conn, char * const buf, size_t count TSRMLS_DC);
#elif PHP_VERSION_ID >= 50400
enum_func_status mysqlnd_qc_receive_replay(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
										   MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC);
enum_func_status mysqlnd_qc_receive_record(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
										   MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC);
size_t mysqlnd_qc_send_record(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
							  MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC);
size_t mysqlnd_qc_send_replay(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
							  MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC);
#endif


typedef struct st_mysqlnd_qc_connection_data
{
	zend_bool			query_active_with_cache;
	char *				query_hash_key;
	size_t				query_hash_key_len;
	zend_bool			should_cache;

#if PHP_VERSION_ID < 50400
	enum_func_status	(*original_receive)(MYSQLND * conn, zend_uchar * buffer, size_t count TSRMLS_DC);
	size_t				(*original_send)(MYSQLND * const conn, char * const buf, size_t count TSRMLS_DC);
	smart_str			*recorded_data;
	size_t				recorded_data_current_position;
#endif
	uint64_t			prev_start_time;
	uint64_t			run_time;
	uint				ttl;
#ifdef NORM_QUERY_TRACE_LOG
	MYSQLND_QC_NORM_QUERY_TRACE_LOG_ENTRY * norm_qt_log_entry;
#endif
	zend_bool			multi_q_enabled;
} MYSQLND_QC_CONNECTION_DATA;


#if PHP_VERSION_ID >= 50400
typedef struct st_mysqlnd_qc_net_data
{
	enum_func_status	(*original_receive)(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
											MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC);
	size_t				(*original_send)(MYSQLND_NET * const net, zend_uchar * const buffer, const size_t count,
										 MYSQLND_STATS * const conn_stats, MYSQLND_ERROR_INFO * const error_info TSRMLS_DC);
	smart_str			*recorded_data;
	size_t				recorded_data_current_position;
} MYSQLND_QC_NET_DATA;
#endif


#define FREE_SMART_STR_AND_CONTAINER_AND_NULL(str, persistent) \
		if ((str)) { \
			smart_str_free_ex((str), (persistent)); \
			mnd_free((str)); \
			(str) = NULL; \
		}


#if PHP_VERSION_ID < 50400
#define MYSQLND_QC_NET_SET_REPLAY(conn) \
	{ \
		(conn)->net->m.receive = mysqlnd_qc_receive_replay; \
		(conn)->net->m.send = mysqlnd_qc_send_replay; \
	}

#define MYSQLND_QC_NET_SET_RECORD(conn) \
	{ \
		(conn)->net->m.receive = mysqlnd_qc_receive_record; \
		(conn)->net->m.send = mysqlnd_qc_send_record; \
	}

#define MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, data_pp) \
	{ \
		(conn)->net->m.receive = (*(data_pp))->original_receive; \
		(conn)->net->m.send = (*(data_pp))->original_send; \
	}

#define MYSQLND_QC_NET_SET_ORIGINAL(conn, data_pp) \
	{ \
		(*(data_pp))->original_receive = (conn)->net->m.receive; \
		(*(data_pp))->original_send = (conn)->net->m.send; \
	}

#elif PHP_VERSION_ID >= 50400 && PHP_VERSION_ID < 50500

#define MYSQLND_QC_NET_SET_REPLAY(conn) \
	{ \
		(conn)->net->m.receive_ex = mysqlnd_qc_receive_replay; \
		(conn)->net->m.send_ex = mysqlnd_qc_send_replay; \
	}

#define MYSQLND_QC_NET_SET_RECORD(conn) \
	{ \
		(conn)->net->m.receive_ex = mysqlnd_qc_receive_record; \
		(conn)->net->m.send_ex = mysqlnd_qc_send_record; \
	}

#define MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, data_pp) \
	{ \
		MYSQLND_QC_NET_DATA ** xx_net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data((conn)->net, mysqlnd_qc_plugin_id); \
		(conn)->net->m.receive_ex = (*(xx_net_data_pp))->original_receive; \
		(conn)->net->m.send_ex = (*(xx_net_data_pp))->original_send; \
	}

#define QC_DECLARE_AND_LOAD_NET_DATA(net_data_pp, conn_data) \
	MYSQLND_QC_NET_DATA ** net_data_pp = \
		(MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data((conn_data)->net, mysqlnd_qc_plugin_id)


#define MYSQLND_QC_NET_SET_ORIGINAL(conn, data_pp) \
	{ \
		MYSQLND_QC_NET_DATA ** xx_net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data((conn)->net, mysqlnd_qc_plugin_id); \
		if ((*xx_net_data_pp) == NULL) { \
			/* TODO: Andrey, persistent? */ \
			*xx_net_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_QC_CONNECTION_DATA), conn->persistent); \
		} \
		(*(xx_net_data_pp))->original_receive = (conn)->net->m.receive_ex; \
		(*(xx_net_data_pp))->original_send = (conn)->net->m.send_ex; \
	}

#elif PHP_VERSION_ID >= 50500

#define MYSQLND_QC_NET_SET_REPLAY(conn) \
	{ \
		(conn)->net->data->m.receive_ex = mysqlnd_qc_receive_replay; \
		(conn)->net->data->m.send_ex = mysqlnd_qc_send_replay; \
	}

#define MYSQLND_QC_NET_SET_RECORD(conn) \
	{ \
		(conn)->net->data->m.receive_ex = mysqlnd_qc_receive_record; \
		(conn)->net->data->m.send_ex = mysqlnd_qc_send_record; \
	}

#define MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, data_pp) \
	{ \
		MYSQLND_QC_NET_DATA ** xx_net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data((conn)->net, mysqlnd_qc_plugin_id); \
		(conn)->net->data->m.receive_ex = (*(xx_net_data_pp))->original_receive; \
		(conn)->net->data->m.send_ex = (*(xx_net_data_pp))->original_send; \
	}

#define QC_DECLARE_AND_LOAD_NET_DATA(net_data_pp, conn_data) \
	MYSQLND_QC_NET_DATA ** net_data_pp = \
		(MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data((conn_data)->net, mysqlnd_qc_plugin_id)


#define MYSQLND_QC_NET_SET_ORIGINAL(conn, data_pp) \
	{ \
		MYSQLND_QC_NET_DATA ** xx_net_data_pp = (MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data((conn)->net, mysqlnd_qc_plugin_id); \
		if ((*xx_net_data_pp) == NULL) { \
			/* TODO: Andrey, persistent? */ \
			*xx_net_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_QC_CONNECTION_DATA), conn->persistent); \
		} \
		(*(xx_net_data_pp))->original_receive = (conn)->net->data->m.receive_ex; \
		(*(xx_net_data_pp))->original_send = (conn)->net->data->m.send_ex; \
	}
#endif  /* 50500 */


#define COPY_HASH_KEY(src_key, src_len, dst_key, dst_len)  dst_key = src_key; dst_len = src_len;

#define TIMEVAL_TO_UINT64(tp) (uint64_t)(tp.tv_sec*1000000 + tp.tv_usec)

#define STATS_TIME_SET(time_now) \
	if (MYSQLND_QC_G(time_statistics) == FALSE) { \
		(time_now) = 0; \
	} else { \
		struct timeval __tp = {0}; \
		struct timezone __tz = {0}; \
		gettimeofday(&__tp, &__tz); \
		(time_now) = TIMEVAL_TO_UINT64(__tp); \
	} \

#define STATS_TIME_DIFF(run_time) \
	{ \
		uint64_t now; \
		STATS_TIME_SET(now); \
		(run_time) = now - (run_time); \
	}

#define STATS_TIME_SET2(time_now, time_now2) \
	if (MYSQLND_QC_G(time_statistics) == FALSE) { \
		(time_now) = 0; \
		(time_now2) = 0; \
	} else { \
		struct timeval __tp = {0}; \
		struct timezone __tz = {0}; \
		gettimeofday(&__tp, &__tz); \
		(time_now) = TIMEVAL_TO_UINT64(__tp); \
		(time_now2) = (time_now); \
	} \

#define STATS_TIME_DIFF2(run_time, run_time2) \
	{ \
		uint64_t now; \
		STATS_TIME_SET(now); \
		(run_time) = now - (run_time); \
		(run_time2) = now - (run_time2); \
	}

#endif	/* MYSQLND_QC_PRIV_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
