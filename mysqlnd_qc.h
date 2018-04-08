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

#ifndef MYSQLND_QC_H
#define MYSQLND_QC_H

#ifdef PHP_WIN32
#define PHP_MYSQLND_QC_API __declspec(dllexport)
#else
# if defined(__GNUC__) && __GNUC__ >= 4
#  define PHP_MYSQLND_QC_API __attribute__ ((visibility("default")))
# else
#  define PHP_MYSQLND_QC_API
# endif
#endif

#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_statistics.h"
#include "ext/mysqlnd/mysqlnd_debug.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#if PHP_VERSION_ID >= 50400
#include "ext/mysqlnd/mysqlnd_ext_plugin.h"
#endif

#ifndef SMART_STR_START_SIZE
#define SMART_STR_START_SIZE 2048
#endif
#ifndef SMART_STR_PREALLOC
#define SMART_STR_PREALLOC 512
#endif
#include "ext/standard/php_smart_str.h"

#include "mysqlnd_qc_tokenize.h"

#ifdef PHP_WIN32
#include "win32/time.h"
#elif defined(NETWARE)
#include <sys/timeval.h>
#include <sys/time.h>
#else
#include <sys/time.h>
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef MYSQLND_QC_HAVE_MEMCACHE
struct memcached_st;
#include <libmemcached/memcached.h>
#endif


#define MYSQLND_QC_GET_HASH_KEY_NAME		"get_hash_key"
#define MYSQLND_QC_FIND_IN_CACHE_NAME		"find_in_cache"
#define MYSQLND_QC_RETURN_TO_CACHE_NAME		"return_to_cache"
#define MYSQLND_QC_ADD_TO_CACHE_NAME		"add_to_cache"
#define MYSQLND_QC_UPDATE_CACHE_STATS_NAME	"update_cache_stats"
#define MYSQLND_QC_FILL_STATS_NAME			"get_stats"
#define MYSQLND_QC_CLEAR_CACHE_NAME			"clear_cache"
#define MYSQLND_QC_INIT_NAME				"init"
#define MYSQLND_QC_SHUTDOWN_NAME			"shutdown"

#define MYSQLND_QC_GET_HASH_KEY_METHOD			get_hash_key
#define MYSQLND_QC_FIND_IN_CACHE_METHOD			find_in_cache
#define MYSQLND_QC_RETURN_TO_CACHE_METHOD		return_to_cache
#define MYSQLND_QC_ADD_TO_CACHE_METHOD			add_to_cache
#define MYSQLND_QC_UPDATE_CACHE_STATS_METHOD	update_cache_stats
#define MYSQLND_QC_FILL_STATS_METHOD			get_stats
#define MYSQLND_QC_CLEAR_CACHE_METHOD			clear_cache
#define MYSQLND_QC_INIT_METHOD					init
#define MYSQLND_QC_SHUTDOWN_METHOD				shutdown


#ifdef ZTS
#define MYSQLND_QC_G(v) TSRMG(mysqlnd_qc_globals_id, zend_mysqlnd_qc_globals *, v)
#else
#define MYSQLND_QC_G(v) (mysqlnd_qc_globals.v)
#endif

#define MYSQLND_QC_ERROR_PREFIX "(mysqlnd_qc)"

#define PHP_MYSQLND_QC_EXPORT(__type) PHP_MYSQLND_QC_API __type

typedef enum mysqlnd_qc_condition_type
{
	QC_CONDITION_META_SCHEMA_PATTERN = 0,
	QC_CONDITION_RUN_TIME,
	QC_CONDITION_LAST
} enum_mysqlnd_qc_condition_type;


typedef struct st_mysqlnd_qc_should_cache_condition {
	/* TODO: decide on global mutex */
	enum_mysqlnd_qc_condition_type	type;
	void *							conditions;
} MYSQLND_QC_SHOULD_CACHE_CONDITION;


typedef struct st_mysqlnd_qc_condition_pattern {
	const char * pattern;
	uint ttl;
} MYSQLND_QC_CONDITION_PATTERN;


#if MYSQLND_VERSION_ID < 50010 && !defined(MYSQLND_CONN_DATA_DEFINED)
#define MYSQLND_CONN_DATA_DEFINED
typedef MYSQLND MYSQLND_CONN_DATA;
#endif


typedef struct st_mysqlnd_qc_methods
{
	const char *		name;
	const char *		version;
	char *				(*get_hash_key)(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC);
	/* TODO: decide wether it checks for timeouts or not */
	zend_bool			(*query_is_cached)(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC);
	smart_str *			(*find_query_in_cache)(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC);
	void				(*return_to_cache)(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC);
	enum_func_status 	(*add_query_to_cache_if_not_exists)(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len, smart_str * recorded_data, uint TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC);
	void				(*update_query_run_time_stats)(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC);
	long				(*fill_stats_hash)(zval *return_value TSRMLS_DC ZEND_FILE_LINE_DC);
	enum_func_status	(*clear_cache)(TSRMLS_D);
	void				(*handler_minit)(TSRMLS_D);
	void				(*handler_mshutdown)(TSRMLS_D);
	enum_func_status	(*handler_change_init)(TSRMLS_D);
	enum_func_status	(*handler_change_shutdown)(TSRMLS_D);
	enum_func_status	(*handler_change_refresh)(TSRMLS_D);
} MYSQLND_QC_METHODS;


struct st_mysqlnd_qc_qcache {
	HashTable	ht;
#ifdef ZTS
	MUTEX_T		LOCK_access;
#endif
};

#ifdef ZTS
#define LOCK_QCACHE(cache)		DBG_INF_FMT("LOCK @ %s::%d", __FUNCTION__, __LINE__);tsrm_mutex_lock((cache).LOCK_access)
#define UNLOCK_QCACHE(cache)	DBG_INF_FMT("UNLOCK @ %s::%d", __FUNCTION__, __LINE__);tsrm_mutex_unlock((cache).LOCK_access)
#else
#define LOCK_QCACHE(cache)
#define UNLOCK_QCACHE(cache)
#endif


#define NUM_USER_HANDLERS 7

ZEND_BEGIN_MODULE_GLOBALS(mysqlnd_qc)
	zend_bool enable_qc;
	long  ttl;
	zend_bool cache_by_default;
	zend_bool cache_no_table;
	zend_bool collect_statistics;
	char* collect_statistics_log_file;
	zend_bool use_request_time;
	zend_bool std_data_copy;
	zend_bool time_statistics;
	zend_bool slam_defense;
	long slam_defense_ttl;
	zval *query_is_select;
	MYSQLND_QC_METHODS * handler;
	union {
		zval *names[NUM_USER_HANDLERS];
		struct {
			zval *get_hash;
			zval *find_in_cache;
			zval *return_to_cache;
			zval *add_to_cache;
			zval *update_query_run_time_stats;
			zval *fill_stats_hash;
			zval *clear_cache;
		} name;
	} user_handlers_names;
	zval * handler_object;
	zval * new_handler_object;
#if MYSQLND_QC_HAVE_APC
	char * apc_prefix;
#endif
#ifdef MYSQLND_QC_HAVE_MEMCACHE
	struct memcached_st * memc;
	char * memc_server;
	int memc_port;
#endif
	char * 		sqlite_data_file;
	zend_llist	query_trace_log;
	zend_bool	collect_query_trace;
	uint		query_trace_bt_depth;
	zend_bool	collect_normalized_query_trace;
	long	 	request_counter;
	ulong		process_hash;
	zend_bool	ignore_sql_comments;
	zend_llist  should_cache_conditions;
ZEND_END_MODULE_GLOBALS(mysqlnd_qc)


void mysqlnd_qc_query_trace_log_entry_dtor_func(void *pDest);
PHP_MYSQLND_QC_API void mysqlnd_qc_get_query_trace_log(zval * return_value TSRMLS_DC);

PHP_MYSQLND_QC_API zend_bool mysqlnd_qc_query_is_cached(MYSQLND_CONN_DATA * conn, const char * const query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC);

extern MYSQLND_STATS * mysqlnd_qc_stats;

typedef enum mysqlnd_qc_collected_stats
{
	QC_STAT_HIT,
	QC_STAT_MISS,
	QC_STAT_PUT,
	QC_STAT_QUERY_SHOULD_CACHE,
	QC_STAT_QUERY_SHOULD_NOT_CACHE,
	QC_STAT_QUERY_NOT_CACHED,
	QC_STAT_QUERY_COULD_CACHE,
	QC_STAT_QUERY_FOUND_IN_CACHE,
	QC_STAT_QUERY_UNCACHED_OTHER,
	QC_STAT_QUERY_UNCACHED_NO_TABLE,
	QC_STAT_QUERY_UNCACHED_NO_RESULT,
	QC_STAT_QUERY_UNCACHED_USE_RESULT,
	QC_STAT_QUERY_AGGR_RUN_TIME_CACHE_HIT,
	QC_STAT_QUERY_AGGR_RUN_TIME_CACHE_PUT,
	QC_STAT_QUERY_AGGR_RUN_TIME_TOTAL,
	QC_STAT_QUERY_AGGR_STORE_TIME_CACHE_HIT,
	QC_STAT_QUERY_AGGR_STORE_TIME_CACHE_PUT,
	QC_STAT_QUERY_AGGR_STORE_TIME_TOTAL,
	QC_STAT_RECEIVE_BYTES_RECORDED,
	QC_STAT_RECEIVE_BYTES_REPLAYED,
	QC_STAT_SEND_BYTES_RECORDED,
	QC_STAT_SEND_BYTES_REPLAYED,
	QC_STAT_SLAM_STALE_REFRESH,
	QC_STAT_SLAM_STALE_HIT,
	QC_STAT_LAST /* Should be always the last */
} enum_mysqlnd_qc_collected_stats;

#define MYSQLND_QC_INC_STATISTIC(stat) MYSQLND_INC_STATISTIC(MYSQLND_QC_G(collect_statistics), mysqlnd_qc_stats, (stat))
#define MYSQLND_QC_INC_STATISTIC_W_VALUE(stat, value) MYSQLND_INC_STATISTIC_W_VALUE(MYSQLND_QC_G(collect_statistics), mysqlnd_qc_stats, (stat), (value))
#define MYSQLND_QC_INC_STATISTIC_W_VALUE2(stat1, value1, stat2, value2) MYSQLND_INC_STATISTIC_W_VALUE2(MYSQLND_QC_G(collect_statistics), mysqlnd_qc_stats, (stat1), (value1), (stat2), (value2))
#define MYSQLND_QC_INC_STATISTIC_W_VALUE3(stat1, value1, stat2, value2, stat3, value3) MYSQLND_INC_STATISTIC_W_VALUE3(MYSQLND_QC_G(collect_statistics), mysqlnd_qc_stats, (stat1), (value1), (stat2), (value2), (stat3), (value3))

extern ZEND_DECLARE_MODULE_GLOBALS(mysqlnd_qc)

#define PERSISTENT_STR 1

#define ENABLE_SWITCH "qc=on"
#define DISABLE_SWITCH "qc=off"
#define ENABLE_SWITCH_TTL "qc_ttl="
#define SERVER_ID_SWITCH "qc_sid="

#ifdef ZTS
extern MUTEX_T LOCK_qc_methods_access;
#define LOCK_QC_METHODS()		tsrm_mutex_lock(LOCK_qc_methods_access)
#define UNLOCK_QC_METHODS()		tsrm_mutex_unlock(LOCK_qc_methods_access)
#else
#define LOCK_QC_METHODS()
#define UNLOCK_QC_METHODS()
#endif

extern struct st_mysqlnd_qc_methods *mysqlnd_qc_methods;
extern unsigned int mysqlnd_qc_plugin_id;

void mysqlnd_qc_register_hooks();

#endif	/* MYSQLND_QC_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
