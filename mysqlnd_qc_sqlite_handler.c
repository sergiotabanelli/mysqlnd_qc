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
#include "mysqlnd_qc_zval_util.h"
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_std_handler.h"
#include "mysqlnd_qc_sqlite_handler.h"
#include "mysqlnd_qc_classes.h"

/* HACK - TODO - FIXME */
#if PHP_WIN32
#include "ext/sqlite3/libsqlite/sqlite3.h"
#else
#include "sqlite3.h"
#endif

/* for SG() */
#include "main/SAPI.h"

#define CURRENT_TIME ((MYSQLND_QC_G(use_request_time)) ? SG(global_request_time) : time(NULL))

#ifdef ZTS
#include "TSRM.h"
static MUTEX_T LOCK_access;
#endif

#ifdef ZTS
#define LOCK_MUTEX(mutex)		DBG_INF_FMT("LOCK @ %s::%d", __FUNCTION__, __LINE__);tsrm_mutex_lock((mutex))
#define UNLOCK_MUTEX(mutex)	DBG_INF_FMT("UNLOCK @ %s::%d", __FUNCTION__, __LINE__);tsrm_mutex_unlock((mutex))
#else
#define LOCK_MUTEX(mutex)
#define UNLOCK_MUTEX(mutex)
#endif

static sqlite3 * db_conn = NULL;

uint64_t	cache_max_run_time;
uint64_t	cache_max_store_time;
uint64_t	cache_min_run_time;
uint64_t	cache_min_store_time;
uint64_t	cache_avg_run_time;
uint64_t	cache_avg_store_time;

#define CREATE_TABLE_PREFIX		"CREATE TABLE "
#define TABLE_NAME				"qcache"
#define TABLE_STRUCTURE			"(qhash BLOB"\
								",qdata BLOB" \
								",deadline INTEGER"		\
								",rows INTEGER"				\
								",orig_run_time INTEGER"	\
								",orig_store_time INTEGER"	\
								",row_count INTEGER"	\
								",hits INTEGER"			\
								",max_run_time INTEGER"	\
								",min_run_time INTEGER"	\
								",avg_run_time INTEGER"	\
								",max_store_time INTEGER"\
								",min_store_time INTEGER" \
								",avg_store_time INTEGER)"
#define CREATE_TABLE_SQL  		CREATE_TABLE_PREFIX TABLE_NAME TABLE_STRUCTURE

static const char * const create_table_sql = CREATE_TABLE_SQL;
static const char * const select_row_sql = "SELECT qdata, deadline FROM qcache WHERE qhash ='%*q'";
static const char * const select_exists_sql = "SELECT deadline FROM qcache WHERE qhash ='%*q'";
static const char * const purge_sql = "DELETE FROM qcache WHERE deadline < %lu";
static const char * const purge_table_sql = "DELETE FROM qcache";


/* {{{ mysqlnd_qc_handler_sqlite_get_hash_key_aux */
static char *
mysqlnd_qc_handler_sqlite_get_hash_key_aux(const char * host_info, int port, int charsetnr, const char * user, const char * db, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	char * query_hash_key = NULL;
	DBG_ENTER("mysqlnd_qc_handler_sqlite_get_hash_key_aux");
	if (!server_id && (!host_info || !user || !db || !query || !query_hash_key_len)) {
		DBG_RETURN(NULL);
	}

	if (server_id) {
		*query_hash_key_len = spprintf(&query_hash_key, 0, "%s|%s", server_id, query);
	} else {
		*query_hash_key_len = spprintf(&query_hash_key, 0, "%s\n%d\n%d\n%s\n%s|%s", host_info, port, charsetnr, user, db, query);
	}
	if (persistent) {
		char * tmp = malloc(*query_hash_key_len + 1);
		memcpy(tmp, query_hash_key, *query_hash_key_len + 1);
		efree(query_hash_key);
		query_hash_key = tmp;
	}

	DBG_INF_FMT("query_hash_key(%d)=%s", *query_hash_key_len, query_hash_key);
	DBG_RETURN(query_hash_key);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_get_hash_key */
static char *
mysqlnd_qc_handler_sqlite_get_hash_key(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	char * query_hash_key = NULL;
	smart_str * stripped_query = mysqlnd_qc_query_strip_comments_and_fix_ws(query, query_len TSRMLS_CC);

	if (stripped_query) {
		query_hash_key = mysqlnd_qc_handler_sqlite_get_hash_key_aux(conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", stripped_query->c, stripped_query->len, query_hash_key_len, server_id, server_id_len, persistent TSRMLS_CC);
		smart_str_free_ex(stripped_query, 0);
		efree(stripped_query);
	} else {
		query_hash_key = mysqlnd_qc_handler_sqlite_get_hash_key_aux(conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", query, query_len, query_hash_key_len, server_id, server_id_len, persistent TSRMLS_CC);
	}

	return query_hash_key;
}
/* }}} */

/* {{{ mysqlnd_qc_handler_sqlite_query_is_cached */
static zend_bool
mysqlnd_qc_handler_sqlite_query_is_cached(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC)
{
	zend_bool ret = FALSE;
	char * query_hash_key = NULL;
	size_t query_hash_key_len;

	DBG_ENTER("mysqlnd_qc_handler_sqlite_query_is_cached");
	query_hash_key = mysqlnd_qc_handler_sqlite_get_hash_key(conn, query, query_len, &query_hash_key_len, server_id, server_id_len, 0 TSRMLS_CC);
	if (query_hash_key && db_conn) {
		sqlite3_stmt * stmt = NULL;
		zend_bool stale = FALSE;
		char *find_sql = sqlite3_mprintf(select_exists_sql, query_hash_key_len, query_hash_key);

		LOCK_MUTEX(LOCK_access);
		if (sqlite3_prepare_v2(db_conn, find_sql, -1, &stmt, NULL) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
			if (sqlite3_column_int(stmt, 0) < CURRENT_TIME) {
				/* too old, delete */
				ret = FALSE;
			} else {
				/* still fresh */
				ret = TRUE;
			}
		}
		sqlite3_free(find_sql);
		if (stmt) {
			sqlite3_finalize(stmt);
		}

		if (FALSE == ret) {
			char * errtext = NULL;
			char * clean_sql = sqlite3_mprintf(purge_sql, (ulong) CURRENT_TIME);
			if (sqlite3_exec(db_conn, clean_sql, NULL, NULL, &errtext) != SQLITE_OK) {
				sqlite3_free(errtext);
			}
			sqlite3_free(clean_sql);
		}
		UNLOCK_MUTEX(LOCK_access);
		efree(query_hash_key);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_find_query_in_cache_aux */
static smart_str *
mysqlnd_qc_handler_sqlite_find_query_in_cache_aux(const char * query_hash_key, size_t query_hash_key_len, zend_bool data_copy TSRMLS_DC)
{
	smart_str * cached_query = NULL;
	DBG_ENTER("mysqlnd_qc_handler_sqlite_find_query_in_cache_aux");

	if (db_conn) {
		sqlite3_stmt * stmt = NULL;
		zend_bool stale = FALSE;
		char *find_sql = sqlite3_mprintf(select_row_sql, query_hash_key_len, query_hash_key);

		LOCK_MUTEX(LOCK_access);
		if (sqlite3_prepare_v2(db_conn, find_sql, -1, &stmt, NULL) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
			if (sqlite3_column_int(stmt, 1) < CURRENT_TIME) {
				/* too old, delete */
				stale = TRUE;
			} else {
				/* still fresh */
				cached_query = mnd_calloc(1, sizeof(smart_str));
				smart_str_appendl_ex(cached_query, sqlite3_column_blob(stmt, 0), sqlite3_column_bytes(stmt, 0), PERSISTENT_STR);
			}
		}
		sqlite3_free(find_sql);
		if (stmt) {
			sqlite3_finalize(stmt);
		}
		if (stale == TRUE) {
			char * errtext = NULL;
			char * clean_sql = sqlite3_mprintf(purge_sql, (ulong) CURRENT_TIME);
			if (sqlite3_exec(db_conn, clean_sql, NULL, NULL, &errtext) != SQLITE_OK) {
				sqlite3_free(errtext);
			}
			sqlite3_free(clean_sql);
		}

		UNLOCK_MUTEX(LOCK_access);
	}

	DBG_RETURN(cached_query);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_find_query_in_cache */
static smart_str *
mysqlnd_qc_handler_sqlite_find_query_in_cache(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC)
{
	return mysqlnd_qc_handler_sqlite_find_query_in_cache_aux(query_hash_key, query_hash_key_len, MYSQLND_QC_G(std_data_copy) TSRMLS_CC);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_return_to_cache */
static void
mysqlnd_qc_handler_sqlite_return_to_cache(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_handler_sqlite_return_to_cache");

	smart_str_free_ex(cached_query, PERSISTENT_STR);
	mnd_free(cached_query);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists */
static enum_func_status
mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len,
															smart_str * recorded_data, uint TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	DBG_ENTER("mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists");
	DBG_INF_FMT("ttl=%d recorded_data_len=%d", TTL, recorded_data->len);

	if (db_conn) {
		zend_bool add = TRUE;
		zend_bool stale = FALSE;
		char *find_sql = sqlite3_mprintf(select_row_sql, query_hash_key_len, query_hash_key);
		char *insert_sql = sqlite3_mprintf("INSERT INTO qcache (qhash, qdata, deadline, rows, orig_run_time, orig_store_time, row_count, "
											"hits, max_run_time, min_run_time, avg_run_time, max_store_time, min_store_time, avg_store_time) "
											"VALUES ('%*q', ?, %lu, %lu, %lu, %lu, %lu,"
											"0, 0, 0, 0, 0, 0, 0)",
											query_hash_key_len, query_hash_key, (ulong) CURRENT_TIME + TTL, (ulong) row_count, (ulong) run_time, (ulong) store_time, (ulong) row_count);
		sqlite3_stmt * stmt = NULL;
		DBG_INF_FMT("find_sql=%s", find_sql);
		DBG_INF_FMT("insert_sql=%s", insert_sql);

		LOCK_MUTEX(LOCK_access);

		if (sqlite3_prepare_v2(db_conn, find_sql, -1, &stmt, NULL) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
			if (sqlite3_column_int(stmt, 1) < CURRENT_TIME) {
				/* too old, delete */
				stale = TRUE;
			} else {
				add = FALSE;
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
		}
		if (stale == TRUE) {
			char * errtext = NULL;
			char * clean_sql = sqlite3_mprintf(purge_sql, CURRENT_TIME);
			if (sqlite3_exec(db_conn, clean_sql, NULL, NULL, &errtext) != SQLITE_OK) {
				sqlite3_free(errtext);
			}
			sqlite3_free(clean_sql);
		}

		if (add == TRUE) {
			sqlite3_stmt * stmt = NULL;
			DBG_INF("INSERT");
			if (sqlite3_prepare_v2(db_conn, insert_sql, -1, &stmt, NULL) == SQLITE_OK &&
				sqlite3_bind_blob(stmt, 1, recorded_data->c, recorded_data->len, SQLITE_TRANSIENT) == SQLITE_OK &&
				sqlite3_step(stmt) == SQLITE_DONE)
			{
				DBG_INF("Prepared and executed");
				ret = PASS;
			}
			if (stmt) {
				sqlite3_finalize(stmt);
			}
		}

		UNLOCK_MUTEX(LOCK_access);
		sqlite3_free(find_sql);
		sqlite3_free(insert_sql);
	}
	if (PASS == ret) {
		smart_str_free_ex(recorded_data, PERSISTENT_STR);
		mnd_free(recorded_data);
	}
	DBG_RETURN(ret);
}
/* }}} */


#define CALC_N_STORE_AVG(current_avg, current_count, new_value) (current_avg) = ((current_avg) * (current_count) + (new_value))/ ((current_count) + 1)

const char * const update_stats_select = "SELECT hits, max_run_time, min_run_time, avg_run_time, max_store_time, min_store_time, avg_store_time FROM qcache";

/* {{{ mysqlnd_qc_handler_sqlite_update_cache_stats */
static void
mysqlnd_qc_handler_sqlite_update_cache_stats(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_handler_sqlite_update_cache_stats");
	if (!query_hash_key) {
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("run_time=%llu store_time=%llu", run_time, store_time);
	if (db_conn) {
		sqlite3_stmt * stmt = NULL;
		zend_bool found = FALSE;
		uint64_t hits = 0, max_run_time = 0, min_run_time = 0, avg_run_time = 0, max_store_time = 0, min_store_time = 0, avg_store_time = 0;
		LOCK_MUTEX(LOCK_access);

		if (sqlite3_prepare_v2(db_conn, update_stats_select, -1, &stmt, NULL) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
			found = TRUE;
			hits = sqlite3_column_int(stmt, 0);
			max_run_time = sqlite3_column_int(stmt, 1);
			min_run_time = sqlite3_column_int(stmt, 2);
			avg_run_time = sqlite3_column_int(stmt, 3);
			max_store_time = sqlite3_column_int(stmt, 4);
			min_store_time = sqlite3_column_int(stmt, 5);
			avg_store_time = sqlite3_column_int(stmt, 6);
		}
		if (stmt) {
			sqlite3_finalize(stmt);
		}
		if (found == TRUE) {
			if (min_run_time == 0) {
				min_run_time = run_time;
			}
			if (min_store_time == 0) {
				min_store_time = store_time;
			}

			if (run_time < min_run_time) {
				min_run_time = run_time;
			} else if (run_time > max_run_time) {
				max_run_time = run_time;
			}

			if (store_time < min_store_time) {
				min_store_time = store_time;
			} else if (store_time > max_store_time) {
				max_store_time = store_time;
			}
			CALC_N_STORE_AVG(avg_run_time, hits, run_time);
			CALC_N_STORE_AVG(avg_store_time, hits, store_time);
			hits++;
			{
				char *update_sql = sqlite3_mprintf("UPDATE qcache SET hits=%lu, max_run_time=%lu, min_run_time=%lu, avg_run_time=%lu,"
									"max_store_time=%lu, min_store_time=%lu, avg_store_time=%lu "
							"WHERE qhash=%*Q",
							(ulong) hits, (ulong) max_run_time, (ulong) min_run_time, (ulong) avg_run_time,
							(ulong) max_store_time, (ulong) min_store_time, (ulong) avg_store_time, query_hash_key_len, query_hash_key);
				char * errtext = NULL;
				DBG_INF_FMT("update=%s", update_sql);
				if (sqlite3_exec(db_conn, update_sql, NULL, NULL, &errtext) != SQLITE_OK) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Update of cache statistics failed: %s", MYSQLND_QC_ERROR_PREFIX, errtext);
					DBG_ERR_FMT("%s", errtext);
					sqlite3_free(errtext);
				}
				sqlite3_free(update_sql);
			}
		}

		UNLOCK_MUTEX(LOCK_access);
	}
	DBG_VOID_RETURN;
}
/* }}} */


const char * stats_sql = "SELECT rows, LENGTH(qdata), hits, orig_run_time, orig_store_time, max_run_time, min_run_time, avg_run_time, max_store_time, min_store_time, avg_store_time, qhash FROM qcache";

/* {{{ mysqlnd_qc_handler_sqlite_fill_stats_hash */
static long
mysqlnd_qc_handler_sqlite_fill_stats_hash(zval * return_value TSRMLS_DC ZEND_FILE_LINE_DC)
{
	long num_entries = 0;
	DBG_ENTER("mysqlnd_qc_handler_sqlite_fill_stats_hash");

	array_init(return_value);
	if (db_conn) {
		uint64_t stored_size = 0, rows = 0, hits = 0, orig_run_time, orig_store_time = 0, max_run_time = 0,
				min_run_time = 0, avg_run_time = 0, max_store_time = 0, min_store_time = 0, avg_store_time = 0;
		sqlite3_stmt * stmt = NULL;
		DBG_INF_FMT("stats_sql=%s", stats_sql);

		LOCK_MUTEX(LOCK_access);
		if (sqlite3_prepare_v2(db_conn, stats_sql, -1, &stmt, NULL) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				zval * stats_array, * inner_array;
				num_entries++;
				DBG_INF("ROW");
				rows = sqlite3_column_int(stmt, 0);
				stored_size = sqlite3_column_int(stmt, 1);
				hits = sqlite3_column_int(stmt, 2);
				orig_run_time = sqlite3_column_int(stmt, 3);
				orig_store_time = sqlite3_column_int(stmt, 4);
				max_run_time = sqlite3_column_int(stmt, 5);
				min_run_time = sqlite3_column_int(stmt, 6);
				avg_run_time = sqlite3_column_int(stmt, 7);
				max_store_time = sqlite3_column_int(stmt, 8);
				min_store_time = sqlite3_column_int(stmt, 9);
				avg_store_time = sqlite3_column_int(stmt, 10);

				MAKE_STD_ZVAL(stats_array);
				array_init(stats_array);
				mysqlnd_qc_add_to_array_long(stats_array, "rows", sizeof("rows") - 1, rows TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "stored_size", sizeof("stored_size") - 1, stored_size TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "cache_hits", sizeof("cache_hits") - 1, hits TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "run_time", sizeof("run_time") - 1, orig_run_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "store_time", sizeof("store_time") - 1, orig_store_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "min_run_time", sizeof("min_run_time") - 1, min_run_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "max_run_time", sizeof("max_run_time") - 1, max_run_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "min_store_time", sizeof("min_store_time") - 1, min_store_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "max_store_time", sizeof("max_store_time") - 1, max_store_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "avg_run_time", sizeof("avg_run_time") - 1, avg_run_time TSRMLS_CC);
				mysqlnd_qc_add_to_array_long(stats_array, "avg_store_time", sizeof("avg_store_time") - 1, avg_store_time TSRMLS_CC);

				MAKE_STD_ZVAL(inner_array);
				array_init(inner_array);

				mysqlnd_qc_add_to_array_zval(inner_array, "statistics", sizeof("statistics") - 1, stats_array TSRMLS_CC);
				mysqlnd_qc_add_to_array_zval(return_value, sqlite3_column_blob(stmt, 11), sqlite3_column_bytes(stmt, 11) , inner_array TSRMLS_CC);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
		}

		UNLOCK_MUTEX(LOCK_access);
	}

	return num_entries;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_clear_cache */
enum_func_status
mysqlnd_qc_handler_sqlite_clear_cache(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_handler_sqlite_clear_cache");
	if (db_conn) {
		char * errtext = NULL;
		LOCK_MUTEX(LOCK_access);
		if (sqlite3_exec(db_conn, purge_table_sql, NULL, NULL, &errtext) != SQLITE_OK) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Clearing cache contents failed: %s", MYSQLND_QC_ERROR_PREFIX, errtext);
			DBG_ERR_FMT("%s", errtext);
			sqlite3_free(errtext);
		}
		UNLOCK_MUTEX(LOCK_access);
	}
	DBG_RETURN(PASS);
}
/* }}} */


#ifdef MYSQLND_QC_EXPORT_CLASS_HANDLER_SQLITE
static void mysqlnd_qc_handler_sqlite_minit(TSRMLS_D);
static void mysqlnd_qc_handler_sqlite_mshutdown(TSRMLS_D);
#endif

/* {{{ mysqlnd_qc_handler_init */
static void
mysqlnd_qc_handler_sqlite_handler_minit(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_handler_init");
#ifdef ZTS
	LOCK_access = tsrm_mutex_alloc();
#endif
#ifdef MYSQLND_QC_EXPORT_CLASS_HANDLER_SQLITE
	mysqlnd_qc_handler_sqlite_minit(TSRMLS_C);
#endif
	if (SQLITE_OK != sqlite3_open(MYSQLND_QC_G(sqlite_data_file), &db_conn)) {
		if (db_conn) {
			sqlite3_close(db_conn);
			db_conn = NULL;
		}
	} else {
		char * errtext = NULL;
		if (sqlite3_exec(db_conn, create_table_sql, NULL, NULL, &errtext) != SQLITE_OK) {
			DBG_ERR_FMT("%s", errtext);
			sqlite3_free(errtext);
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_shutdown */
static void
mysqlnd_qc_handler_sqlite_handler_mshutdown(TSRMLS_D)
{
	if (db_conn) {
		sqlite3_close(db_conn);
		db_conn = NULL;
	}
#ifdef ZTS
	tsrm_mutex_free(LOCK_access);
#endif

#ifdef MYSQLND_QC_EXPORT_CLASS_HANDLER_SQLITE
	mysqlnd_qc_handler_sqlite_mshutdown(TSRMLS_C);
#endif
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_sqlite) = {
	MYSQLND_QC_SQLITE_HANDLER_NAME,
	MYSQLND_QC_SQLITE_HANDLER_VERSION_STR,
	mysqlnd_qc_handler_sqlite_get_hash_key,
	mysqlnd_qc_handler_sqlite_query_is_cached,
	mysqlnd_qc_handler_sqlite_find_query_in_cache,
	mysqlnd_qc_handler_sqlite_return_to_cache,
	mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists,
	mysqlnd_qc_handler_sqlite_update_cache_stats,
	mysqlnd_qc_handler_sqlite_fill_stats_hash,
	mysqlnd_qc_handler_sqlite_clear_cache,
	mysqlnd_qc_handler_sqlite_handler_minit,
	mysqlnd_qc_handler_sqlite_handler_mshutdown,
	NULL, /* handler_change_init */
	NULL,  /* handler_change_shutdown */
	NULL  /* handler_change_refresh */
MYSQLND_CLASS_METHODS_END;


#ifdef MYSQLND_QC_EXPORT_CLASS_HANDLER_SQLITE
/* {{{ proto bool mysqlnd_qc_handler_sqlite_get_hash_key_phpfe()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_get_hash_key)
{
	char *host_info, *user, *db, *query, *server_id;
	int host_info_len, port, user_len, db_len, query_len, server_id_len;
	zend_bool persistent;
	zval  *obj;
	size_t query_hash_key_len;
	char * query_hash_key;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_get_hash_key");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oslssssb", &obj, mysqlnd_qc_handler_sqlite_class_entry,
										&host_info, &host_info_len, &port, &user, &user_len, &db, &db_len, &query, &query_len, &server_id, &server_id_len, &persistent) == FAILURE) {
		DBG_VOID_RETURN;
	}
	query_hash_key = mysqlnd_qc_handler_sqlite_get_hash_key_aux(host_info, port, user, db, query, query_len, &query_hash_key_len, server_id, server_id_len, FALSE TSRMLS_CC);
	if (query_hash_key) {
		RETVAL_STRINGL(query_hash_key, query_hash_key_len, 0);
	} else {
		RETVAL_STRINGL("", 0, 1);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_return_to_cache()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_return_to_cache)
{
	zval  *obj;
	int query_hash_key_len;
	char * query_hash_key;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_return_to_cache");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &obj, mysqlnd_qc_handler_sqlite_class_entry,
										&query_hash_key, &query_hash_key_len) == FAILURE) {
		DBG_VOID_RETURN;
	}
	/* just do nothing, the object wrapper will free the query anyway */
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_find_query_in_cache()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_find_query_in_cache)
{
	zval  *obj;
	int query_hash_key_len;
	char * query_hash_key;
	smart_str * data;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_find_query_in_cache");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &obj, mysqlnd_qc_handler_sqlite_class_entry,
										&query_hash_key, &query_hash_key_len) == FAILURE) {
		DBG_VOID_RETURN;
	}
	data = mysqlnd_qc_handler_sqlite_find_query_in_cache_aux(query_hash_key, query_hash_key_len, TRUE TSRMLS_CC);
	if (data) {
		RETVAL_STRINGL(data->c, data->len, 1);
		smart_str_free_ex(data, PERSISTENT_STR);
		mnd_free(data);
	} else {
		RETVAL_NULL();
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists)
{
	zval  *obj;
	int query_hash_key_len, data_len;
	char * query_hash_key, *data;
	int TTL;
	long run_time = 0, store_time = 0;
	smart_str * entry;
	long row_count = 0;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossllll", &obj, mysqlnd_qc_handler_sqlite_class_entry,
									&query_hash_key, &query_hash_key_len, &data, &data_len, &TTL, &run_time, &store_time, &row_count) == FAILURE) {
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("query_hash_key=%p query_hash_key=%s _len=%d data=%p data=%s _len=%d TTL=%d run_time=%d store_time=%d row_count=%d",
			query_hash_key, query_hash_key, query_hash_key_len, data, data, data_len, TTL, run_time, store_time, row_count);
	entry = mnd_calloc(1, sizeof(smart_str));
	smart_str_appendl_ex(entry, data? data:"", data? data_len:0, PERSISTENT_STR);
	if (PASS != mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists(NULL, query_hash_key, query_hash_key_len, entry, TTL, run_time, store_time, row_count TSRMLS_CC)) {
		smart_str_free_ex(entry, PERSISTENT_STR);
		mnd_free(entry);
		RETVAL_FALSE;
	} else {
		RETVAL_TRUE;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_update_cache_stats()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_update_cache_stats)
{
	zval  *obj;
	int query_hash_key_len;
	char * query_hash_key;
	long run_time, store_time;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_update_cache_stats");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll", &obj, mysqlnd_qc_handler_sqlite_class_entry, &query_hash_key, &query_hash_key_len, &run_time, &store_time) == FAILURE) {
		DBG_VOID_RETURN;
	}
	mysqlnd_qc_handler_sqlite_update_cache_stats(query_hash_key, query_hash_key_len, run_time, store_time TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_get_stats()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_get_stats)
{
	zval  *obj;
	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_get_stats");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_sqlite_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	mysqlnd_qc_handler_sqlite_fill_stats_hash(return_value TSRMLS_CC ZEND_FILE_LINE_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_clear_cache()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_clear_cache)
{
	zval  *obj;
	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_clear_cache");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_sqlite_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETURN_BOOL(PASS == mysqlnd_qc_handler_sqlite_clear_cache(TSRMLS_C));
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_init()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_init)
{
	zval  *obj;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_init");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_sqlite_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_TRUE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_sqlite_shutdown()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_sqlite_shutdown)
{
	zval  *obj;

	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_shutdown");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_sqlite_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_TRUE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ handler_sqlite_read_entries_count */
static int
handler_sqlite_read_entries_count(mysqlnd_qc_handler_object * notused, zval **retval TSRMLS_DC)
{
	DBG_ENTER("handler_sqlite_read_entries_count");
	MAKE_STD_ZVAL(*retval);
	ZVAL_LONG(*retval, 0);
	LOCK_MUTEX(LOCK_access);
	if (db_conn) {
		sqlite3_stmt * stmt = NULL;
		if (sqlite3_prepare_v2(db_conn, "SELECT COUNT(*) FROM qcache", -1, &stmt, NULL) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
			ZVAL_LONG(*retval, sqlite3_column_int(stmt, 0));
		}
		if (stmt) {
			sqlite3_finalize(stmt);
		}
	}
	UNLOCK_MUTEX(LOCK_access);
	DBG_RETURN(SUCCESS);
}
/* }}} */



/* {{{ proto object mysqlnd_qc_handler_sqlite_construct() */
PHP_FUNCTION(mysqlnd_qc_handler_sqlite_construct)
{
	DBG_ENTER("zif_mysqlnd_qc_handler_sqlite_construct");
	DBG_VOID_RETURN;
}
/* }}} */

static HashTable mysqlnd_qc_handler_sqlite_properties;
zend_class_entry * mysqlnd_qc_handler_sqlite_class_entry;

/* {{{ mysqlnd_qc_handler_sqlite_methods[] */
const zend_function_entry mysqlnd_qc_handler_sqlite_methods[] = {
	PHP_FALIAS(MYSQLND_QC_GET_HASH_KEY_METHOD,		mysqlnd_qc_handler_sqlite_get_hash_key, NULL)
	PHP_FALIAS(MYSQLND_QC_FIND_IN_CACHE_METHOD,		mysqlnd_qc_handler_sqlite_find_query_in_cache, NULL)
	PHP_FALIAS(MYSQLND_QC_RETURN_TO_CACHE_METHOD,	mysqlnd_qc_handler_sqlite_return_to_cache, NULL)
	PHP_FALIAS(MYSQLND_QC_ADD_TO_CACHE_METHOD,		mysqlnd_qc_handler_sqlite_add_query_to_cache_if_not_exists, NULL)
	PHP_FALIAS(MYSQLND_QC_UPDATE_CACHE_STATS_METHOD,mysqlnd_qc_handler_sqlite_update_cache_stats, NULL)
	PHP_FALIAS(MYSQLND_QC_FILL_STATS_METHOD,		mysqlnd_qc_handler_sqlite_get_stats, NULL)
	PHP_FALIAS(MYSQLND_QC_CLEAR_CACHE_METHOD,		mysqlnd_qc_handler_sqlite_clear_cache, NULL)
	PHP_FALIAS(MYSQLND_QC_INIT_METHOD,				mysqlnd_qc_handler_sqlite_init, NULL)
	PHP_FALIAS(MYSQLND_QC_SHUTDOWN_METHOD,			mysqlnd_qc_handler_sqlite_shutdown, NULL)
	{NULL, NULL, NULL}
};
/* }}} */

static const mysqlnd_qc_handler_property_entry mysqlnd_qc_handler_sqlite_property_entries[] = {
	{"entries",		sizeof("entries")-1,	handler_sqlite_read_entries_count, NULL},
	{NULL, 0, NULL, NULL}
};

/* should not be const, as it is patched during runtime */
static zend_property_info mysqlnd_qc_handler_sqlite_property_info_entries[] = {
	{ZEND_ACC_PUBLIC, "entries",	sizeof("entries") - 1,		0, NULL, 0, NULL},
	{0,					NULL, 		0,							0, NULL, 0, NULL}
};

/* {{{ mysqlnd_qc_handler_sqlite_minit */
static void
mysqlnd_qc_handler_sqlite_minit(TSRMLS_D)
{
	REGISTER_MYSQLND_QC_CLASS("mysqlnd_qc_handler_sqlite", mysqlnd_qc_handler_sqlite_class_entry, mysqlnd_qc_handler_sqlite_methods, NULL, "mysqlnd_qc_handler");
	zend_hash_init(&mysqlnd_qc_handler_sqlite_properties, 0, NULL, NULL, 1);
	MYSQLND_QC_ADD_PROPERTIES(&mysqlnd_qc_handler_sqlite_properties, mysqlnd_qc_handler_sqlite_property_entries);
	MYSQLND_QC_ADD_PROPERTIES_INFO(mysqlnd_qc_handler_sqlite_class_entry, mysqlnd_qc_handler_sqlite_property_info_entries);

	zend_hash_add(&mysqlnd_qc_classes, mysqlnd_qc_handler_sqlite_class_entry->name, mysqlnd_qc_handler_sqlite_class_entry->name_length + 1,
					&mysqlnd_qc_handler_sqlite_properties, sizeof(mysqlnd_qc_handler_sqlite_properties), NULL);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_sqlite_mshutdown */
static void
mysqlnd_qc_handler_sqlite_mshutdown(TSRMLS_D)
{
	zend_hash_destroy(&mysqlnd_qc_handler_sqlite_properties);
}
/* }}} */

#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
