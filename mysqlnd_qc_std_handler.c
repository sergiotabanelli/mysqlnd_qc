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
#include "mysqlnd_qc_classes.h"

/* for SG() */
#include "main/SAPI.h"

#define CURRENT_TIME ((MYSQLND_QC_G(use_request_time)) ? SG(global_request_time) : time(NULL))

#ifdef ZTS
#include "TSRM.h"
#endif

#define QC_SLAM_DEFENSE 1

typedef struct st_mysqlnd_qc_cache_entry {
	smart_str * cached_query;
	size_t		rows;
	uint64_t	deadline;
#ifdef QC_SLAM_DEFENSE
	zend_bool	in_refresh;
#endif
	MYSQLND_RES_METADATA * metadata;
	uint		references;
	uint		cache_hits;
	uint64_t	run_time;
	uint64_t	store_time;
	uint64_t	cache_max_run_time;
	uint64_t	cache_max_store_time;
	uint64_t	cache_min_run_time;
	uint64_t	cache_min_store_time;
	uint64_t	cache_avg_run_time;
	uint64_t	cache_avg_store_time;
} MYSQLND_QC_CACHE_ENTRY;

static struct st_mysqlnd_qc_qcache mysqlnd_qc_qcache;

/* {{{ mysqlnd_qc_hash_element_dtor_func */
static void
mysqlnd_qc_hash_element_dtor_func(void *pDest)
{
	MYSQLND_QC_CACHE_ENTRY * cache_entry = (MYSQLND_QC_CACHE_ENTRY *) pDest;
	TSRMLS_FETCH();
	if (cache_entry->references == 0) {
		smart_str_free_ex(cache_entry->cached_query, PERSISTENT_STR);
		if (cache_entry->metadata) {
			cache_entry->metadata->m->free_metadata(cache_entry->metadata TSRMLS_CC);
		}
		mnd_free(cache_entry->cached_query);
	}
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_get_hash_key_aux */
static char *
mysqlnd_qc_handler_default_get_hash_key_aux(const char * host_info, int port, int charsetnr, const char * user, const char * db, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	char * query_hash_key = NULL;
	DBG_ENTER("mysqlnd_qc_handler_default_get_hash_key_aux");
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


/* {{{ mysqlnd_qc_handler_default_get_hash_key */
static char *
mysqlnd_qc_handler_default_get_hash_key(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{

	char * query_hash_key = NULL;
	smart_str * stripped_query = mysqlnd_qc_query_strip_comments_and_fix_ws(query, query_len TSRMLS_CC);

	if (stripped_query) {
		query_hash_key = mysqlnd_qc_handler_default_get_hash_key_aux(conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", stripped_query->c, stripped_query->len, query_hash_key_len, server_id, server_id_len, persistent TSRMLS_CC);
		smart_str_free_ex(stripped_query, 0);
		efree(stripped_query);
	} else {
		query_hash_key = mysqlnd_qc_handler_default_get_hash_key_aux(conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", query, query_len, query_hash_key_len, server_id, server_id_len, persistent TSRMLS_CC);
	}

	return query_hash_key;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_get_hash_key_phpfe()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_get_hash_key)
{
	char *host_info, *user, *db, *query;
	int host_info_len, user_len, db_len, query_len;
	long port, charsetnr;
	zend_bool persistent;
	zval  *obj;
	size_t query_hash_key_len;
	char * query_hash_key;
	char * server_id = NULL;
	size_t server_id_len = 0;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_get_hash_key");
	/* TODO: add server_id support */

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osllsssb", &obj, mysqlnd_qc_handler_default_class_entry,
										&host_info, &host_info_len, &port, &charsetnr, &user, &user_len, &db, &db_len, &query, &query_len, &persistent) == FAILURE) {
		DBG_VOID_RETURN;
	}
	query_hash_key = mysqlnd_qc_handler_default_get_hash_key_aux(host_info, port, charsetnr, user, db, query, query_len, &query_hash_key_len, server_id, server_id_len, FALSE TSRMLS_CC);
	if (query_hash_key) {
		RETVAL_STRINGL(query_hash_key, query_hash_key_len, 0);
	} else {
		RETVAL_STRINGL("", 0, 1);
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_query_is_cached */
static zend_bool
mysqlnd_qc_handler_default_query_is_cached(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC)
{
	zend_bool ret = FALSE;
	size_t query_hash_key_len;
	char * query_hash_key;
	MYSQLND_QC_CACHE_ENTRY * cache_entry;

	DBG_ENTER("mysqlnd_qc_handler_default_query_is_cached");
	/* TODO: add server_id */
	query_hash_key = mysqlnd_qc_handler_default_get_hash_key(conn, query, query_len, &query_hash_key_len, server_id, server_id_len, 0 TSRMLS_CC);
	if (query_hash_key) {
		LOCK_QCACHE(mysqlnd_qc_qcache);
		if (SUCCESS == zend_hash_find(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1, (void **) &cache_entry)) {
			if (cache_entry->deadline <= CURRENT_TIME
#ifdef QC_SLAM_DEFENSE
			&& !MYSQLND_QC_G(slam_defense)
#endif
				)
			{
				if (cache_entry->references == 0) {
					zend_hash_del(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1);
				}
				ret = FALSE;
			} else {
				ret = TRUE;
			}
		}
		UNLOCK_QCACHE(mysqlnd_qc_qcache);
		efree(query_hash_key);
	}
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_find_query_in_cache_aux */
static smart_str *
mysqlnd_qc_handler_default_find_query_in_cache_aux(const char * query_hash_key, size_t query_hash_key_len, zend_bool data_copy TSRMLS_DC)
{
	smart_str * cached_query = NULL;
	MYSQLND_QC_CACHE_ENTRY * cache_entry;
	DBG_ENTER("mysqlnd_qc_handler_default_find_query_in_cache_aux");

	LOCK_QCACHE(mysqlnd_qc_qcache);
	if (SUCCESS == zend_hash_find(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1, (void **) &cache_entry)) {
		if (cache_entry->deadline <= CURRENT_TIME
#ifdef QC_SLAM_DEFENSE
		 && !MYSQLND_QC_G(slam_defense)
#endif
		 	)
		{
			DBG_INF("Time to kill");
			if (cache_entry->references == 0) {
				zend_hash_del(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1);
			} else {
				DBG_INF("Someone else will kill it");
			}
		} else {
#ifdef QC_SLAM_DEFENSE
			if (MYSQLND_QC_G(slam_defense) && !cache_entry->in_refresh) {
				cache_entry->in_refresh = TRUE;
				cached_query = NULL; /* ask this thread to refresh and everyone else will get the stale copy */
				MYSQLND_QC_INC_STATISTIC(QC_STAT_SLAM_STALE_REFRESH);
			} else
#endif
			{
#ifdef QC_SLAM_DEFENSE
				if (MYSQLND_QC_G(slam_defense)) {
					MYSQLND_QC_INC_STATISTIC(QC_STAT_SLAM_STALE_HIT);
				}
#endif
				if (!data_copy) {
					cached_query = cache_entry->cached_query;
					++cache_entry->references;
					++cache_entry->cache_hits;
				} else {
					cached_query = mnd_calloc(1, sizeof(smart_str));
					smart_str_appendl_ex(cached_query, cache_entry->cached_query->c, cache_entry->cached_query->len, PERSISTENT_STR);
					DBG_INF_FMT("Data copied: %d", cache_entry->cached_query->len);
				}
			}
		}
	}
	UNLOCK_QCACHE(mysqlnd_qc_qcache);

	DBG_RETURN(cached_query);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_find_query_in_cache */
static smart_str *
mysqlnd_qc_handler_default_find_query_in_cache(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC)
{
	return mysqlnd_qc_handler_default_find_query_in_cache_aux(query_hash_key, query_hash_key_len, MYSQLND_QC_G(std_data_copy) TSRMLS_CC);
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_find_query_in_cache()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_find_query_in_cache)
{
	zval  *obj;
	int query_hash_key_len;
	char * query_hash_key;
	smart_str * data;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_get_hash_key");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &obj, mysqlnd_qc_handler_default_class_entry,
										&query_hash_key, &query_hash_key_len) == FAILURE) {
		DBG_VOID_RETURN;
	}
	data = mysqlnd_qc_handler_default_find_query_in_cache_aux(query_hash_key, query_hash_key_len, TRUE TSRMLS_CC);
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


/* {{{ mysqlnd_qc_handler_default_return_to_cache */
static void
mysqlnd_qc_handler_default_return_to_cache(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC)
{
	MYSQLND_QC_CACHE_ENTRY * tmp_entry;
	DBG_ENTER("mysqlnd_qc_handler_default_return_to_cache");

	LOCK_QCACHE(mysqlnd_qc_qcache);
	if (!MYSQLND_QC_G(std_data_copy)) {
		if (SUCCESS == zend_hash_find(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1, (void **) &tmp_entry)) {
			DBG_INF_FMT("Found it in the cache. Before decrease ref=%u", tmp_entry->references);
			tmp_entry->references--;
			if (tmp_entry->references == 0 && tmp_entry->deadline <= CURRENT_TIME
#ifdef QC_SLAM_DEFENSE
				 && (!MYSQLND_QC_G(slam_defense) || !tmp_entry->in_refresh)
#endif
				 )
			{
				DBG_INF("Time to kill");
				zend_hash_del(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1);
			}
		} else {
			/*
			  We got it from the cache but can't find it.
			  This should be the result of `clear_cache`. Just decrease the refcounter and if 0, free.
			*/
		}
	} else {
		smart_str_free_ex(cached_query, PERSISTENT_STR);
		mnd_free(cached_query);
	}
	UNLOCK_QCACHE(mysqlnd_qc_qcache);
	/* no free, because zend_hash_del will do it */

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_return_to_cache()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_return_to_cache)
{
	zval  *obj;
	int query_hash_key_len;
	char * query_hash_key;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_return_to_cache");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &obj, mysqlnd_qc_handler_default_class_entry,
										&query_hash_key, &query_hash_key_len) == FAILURE) {
		DBG_VOID_RETURN;
	}
	/* just do nothing, the object wrapper will free the query anyway */
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists */
static enum_func_status
mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len,
															smart_str * recorded_data, uint TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	MYSQLND_QC_CACHE_ENTRY * cache_entry, local_cache_entry = {0};
	zend_bool add = TRUE;
	DBG_ENTER("mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists");
	DBG_INF_FMT("ttl=%d", TTL);

	LOCK_QCACHE(mysqlnd_qc_qcache);
	if (SUCCESS == zend_hash_find(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1, (void **) &cache_entry)) {
		if (cache_entry->deadline < CURRENT_TIME && cache_entry->references == 0
#ifdef QC_SLAM_DEFENSE
			&& (!MYSQLND_QC_G(slam_defense) || cache_entry->in_refresh)
#endif
			)
		{
			DBG_INF("Time to die");
			zend_hash_del(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1);
		} else {
			add = FALSE;
		}
	}
	if (add == TRUE) {
		int op_result;
		memset(&local_cache_entry, 0, sizeof(local_cache_entry));
		local_cache_entry.cached_query = recorded_data;
		local_cache_entry.deadline = CURRENT_TIME + TTL;
		local_cache_entry.run_time = run_time;
		local_cache_entry.store_time = store_time;
		local_cache_entry.metadata = result? result->meta->m->clone_metadata(result->meta, TRUE TSRMLS_CC): NULL;
		local_cache_entry.rows = row_count;

		op_result = zend_hash_add(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1, &local_cache_entry, sizeof(MYSQLND_QC_CACHE_ENTRY), NULL);
		ret = (op_result == SUCCESS)? PASS:FAIL;
		DBG_INF_FMT("Added len = %d", recorded_data->len);
	} else {
		DBG_INF("Skip adding");
		ret = FAIL;
	}
	UNLOCK_QCACHE(mysqlnd_qc_qcache);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists)
{
	zval  *obj;
	int query_hash_key_len, data_len;
	char * query_hash_key, *data;
	long TTL;
	long run_time = 0, store_time = 0;
	smart_str * entry;
	long row_count = 0;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossllll", &obj, mysqlnd_qc_handler_default_class_entry,
									&query_hash_key, &query_hash_key_len, &data, &data_len, &TTL, &run_time, &store_time, &row_count) == FAILURE) {
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("query_hash_key=%p query_hash_key=%s _len=%d data=%p data=%s _len=%d TTL=%d run_time=%d store_time=%d row_count=%d",
			query_hash_key, query_hash_key, query_hash_key_len, data, data, data_len, TTL, run_time, store_time, row_count);
	entry = mnd_calloc(1, sizeof(smart_str));
	smart_str_appendl_ex(entry, data? data:"", data? data_len:0, PERSISTENT_STR);
	if (PASS != mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists(NULL, query_hash_key, query_hash_key_len, entry, TTL, run_time, store_time, row_count TSRMLS_CC)) {
		smart_str_free_ex(entry, PERSISTENT_STR);
		mnd_free(entry);
		RETVAL_FALSE;
	} else {
		RETVAL_TRUE;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_query_is_select */
zend_bool
mysqlnd_qc_handler_default_query_is_select(const char * query, size_t query_len, uint * ttl, char ** server_id, size_t * server_id_len TSRMLS_DC)
{
	zend_bool ret = MYSQLND_QC_G(cache_by_default)? TRUE : FALSE;
	const char * p = query;
	size_t len = query_len;
	struct st_qc_token_and_value token;
	const MYSQLND_CHARSET * cset = mysqlnd_find_charset_name("utf8");

	DBG_ENTER("zif_mysqlnd_qc_handler_default_query_is_select");
	DBG_INF_FMT("query_len="MYSQLND_SZ_T_SPEC, query_len);
	if (!query) {
		DBG_RETURN(FALSE);
	}

	*ttl = 0;
	token = mysqlnd_qc_get_token(&p, &len, cset TSRMLS_CC);

	while (token.token == QC_TOKEN_COMMENT) {
		if (!MYSQLND_QC_G(cache_by_default)) {
			if (ret == FALSE && !strncasecmp(Z_STRVAL(token.value), ENABLE_SWITCH, sizeof(ENABLE_SWITCH) - 1)) {
				ret = TRUE;
			} else if (!strncasecmp(Z_STRVAL(token.value), ENABLE_SWITCH_TTL, sizeof(ENABLE_SWITCH_TTL) - 1)) {
				*ttl = atoi(Z_STRVAL(token.value) + sizeof(ENABLE_SWITCH_TTL) - 1);
			} else if (!strncasecmp(Z_STRVAL(token.value), SERVER_ID_SWITCH, sizeof(SERVER_ID_SWITCH) - 1)) {
				if (*server_id) {
					efree(*server_id);
				}
				*server_id_len = spprintf(server_id, 0, "%s", Z_STRVAL(token.value) + sizeof(SERVER_ID_SWITCH) - 1);
			}
		}
		if (ret == TRUE && !strncasecmp(Z_STRVAL(token.value), DISABLE_SWITCH, sizeof(DISABLE_SWITCH) - 1)) {
			ret = FALSE;
		}
		zval_dtor(&token.value);
		token = mysqlnd_qc_get_token(&p, &len, cset TSRMLS_CC);
	}
	ret = ret && (token.token == QC_TOKEN_SELECT);
	zval_dtor(&token.value);
	DBG_INF_FMT("ret=%d ttl=%d server_id=%s", ret, *ttl, *server_id);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ proto array mysqlnd_qc_default_query_is_select()
 */
PHP_FUNCTION(mysqlnd_qc_default_query_is_select)
{
	int query_hash_key_len;
	char * query_hash_key;
	char * server_id = NULL;
	size_t server_id_len = 0;
	uint ttl;
	zend_bool ret;

	DBG_ENTER("zif_mysqlnd_qc_default_query_is_select");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "s", &query_hash_key, &query_hash_key_len) == FAILURE) {
		DBG_VOID_RETURN;
	}

	ret = mysqlnd_qc_handler_default_query_is_select(query_hash_key, (size_t) query_hash_key_len, &ttl, &server_id,  &server_id_len TSRMLS_CC);

	if (ret == FALSE) {
		RETVAL_FALSE;
	} else {
		array_init(return_value);
		add_assoc_long(return_value, "ttl", ttl);
		if (server_id) {
			add_assoc_stringl(return_value, "server_id", server_id, server_id_len, 1);
			efree(server_id);
		} else {
			add_assoc_null(return_value, "server_id");
		}
	}

	DBG_VOID_RETURN;
}
/* }}} */


#define CALC_N_STORE_AVG(current_avg, current_count, new_value) (current_avg) = ((current_avg) * (current_count) + (new_value))/ ((current_count) + 1)

/* {{{ mysqlnd_qc_handler_default_update_cache_stats */
static void
mysqlnd_qc_handler_default_update_cache_stats(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC)
{
	MYSQLND_QC_CACHE_ENTRY * cache_entry;
	DBG_ENTER("mysqlnd_qc_handler_default_update_cache_stats");
	if (!query_hash_key) {
		DBG_VOID_RETURN;
	}

	DBG_INF_FMT("run_time=%llu store_time=%llu", run_time, store_time);
	LOCK_QCACHE(mysqlnd_qc_qcache);
	if (SUCCESS == zend_hash_find(&mysqlnd_qc_qcache.ht, query_hash_key, query_hash_key_len + 1, (void **) &cache_entry)) {
		if (cache_entry->cache_min_run_time == 0) {
			cache_entry->cache_min_run_time = run_time;
		}
		if (cache_entry->cache_min_store_time == 0) {
			cache_entry->cache_min_store_time = store_time;
		}

		if (run_time < cache_entry->cache_min_run_time) {
			cache_entry->cache_min_run_time = run_time;
		} else if (run_time > cache_entry->cache_max_run_time) {
			cache_entry->cache_max_run_time = run_time;
		}

		if (store_time < cache_entry->cache_min_store_time) {
			cache_entry->cache_min_store_time = store_time;
		} else if (store_time > cache_entry->cache_max_store_time) {
			cache_entry->cache_max_store_time = store_time;
		}
		CALC_N_STORE_AVG(cache_entry->cache_avg_run_time, cache_entry->cache_hits, run_time);
		CALC_N_STORE_AVG(cache_entry->cache_avg_store_time, cache_entry->cache_hits, store_time);
	}
	UNLOCK_QCACHE(mysqlnd_qc_qcache);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_update_cache_stats()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_update_cache_stats)
{
	zval  *obj;
	int query_hash_key_len;
	char * query_hash_key;
	long run_time, store_time;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_update_cache_stats");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll", &obj, mysqlnd_qc_handler_default_class_entry, &query_hash_key, &query_hash_key_len, &run_time, &store_time) == FAILURE) {
		DBG_VOID_RETURN;
	}
	mysqlnd_qc_handler_default_update_cache_stats(query_hash_key, query_hash_key_len, run_time, store_time TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_fill_stats_hash */
static long
mysqlnd_qc_handler_default_fill_stats_hash(zval *return_value TSRMLS_DC ZEND_FILE_LINE_DC)
{
	long num_entries;
	DBG_ENTER("mysqlnd_qc_handler_default_fill_stats_hash");

	array_init(return_value);
	LOCK_QCACHE(mysqlnd_qc_qcache);
	{
		MYSQLND_QC_CACHE_ENTRY * cache_entry;
		HashPosition pos;
		num_entries = (long)zend_hash_num_elements(&mysqlnd_qc_qcache.ht);
		zend_hash_internal_pointer_reset_ex(&mysqlnd_qc_qcache.ht, &pos);
		while (zend_hash_get_current_data_ex(&mysqlnd_qc_qcache.ht, (void **)&cache_entry, &pos) == SUCCESS) {
			char *str_key;
			uint str_key_len;
			ulong num_key;
			zval * inner_array, * stats_array;
			int key_type = zend_hash_get_current_key_ex(&mysqlnd_qc_qcache.ht, &str_key, &str_key_len, &num_key, 0, &pos);
			switch (key_type) {
				case HASH_KEY_IS_STRING:
					break;
				default:
					/* impossible */
					continue;
			}
			MAKE_STD_ZVAL(stats_array);
			array_init(stats_array);
			mysqlnd_qc_add_to_array_long(stats_array, "rows", sizeof("rows") - 1, cache_entry->rows TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "stored_size", sizeof("stored_size") - 1, cache_entry->cached_query->len TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "cache_hits", sizeof("cache_hits") - 1, cache_entry->cache_hits TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "run_time", sizeof("run_time") - 1, cache_entry->run_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "store_time", sizeof("store_time") - 1, cache_entry->store_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "min_run_time", sizeof("min_run_time") - 1, cache_entry->cache_min_run_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "max_run_time", sizeof("max_run_time") - 1, cache_entry->cache_max_run_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "min_store_time", sizeof("min_store_time") - 1, cache_entry->cache_min_store_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "max_store_time", sizeof("max_store_time") - 1, cache_entry->cache_max_store_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "avg_run_time", sizeof("avg_run_time") - 1, cache_entry->cache_avg_run_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "avg_store_time", sizeof("avg_store_time") - 1, cache_entry->cache_avg_store_time TSRMLS_CC);
			mysqlnd_qc_add_to_array_long(stats_array, "valid_until", sizeof("valid_until") - 1, cache_entry->deadline TSRMLS_CC);

			MAKE_STD_ZVAL(inner_array);
			array_init(inner_array);

			mysqlnd_qc_add_to_array_zval(inner_array, "statistics", sizeof("statistics") - 1, stats_array TSRMLS_CC);
			{
				unsigned int i;
				zval * meta_array;
				MAKE_STD_ZVAL(meta_array);
				array_init(meta_array);
				for (i = 0; i < cache_entry->metadata->field_count; i++) {
					const MYSQLND_FIELD * const field = cache_entry->metadata->m->fetch_field_direct(cache_entry->metadata, i TSRMLS_CC);
					zval * prop_array;
					MAKE_STD_ZVAL(prop_array);
					array_init(prop_array);
					mysqlnd_qc_add_to_array_string(prop_array, "name", sizeof("name") - 1, field->name, field->name_length TSRMLS_CC);
					mysqlnd_qc_add_to_array_string(prop_array, "orig_name", sizeof("orig_name") - 1, field->org_name, field->org_name_length TSRMLS_CC);
					mysqlnd_qc_add_to_array_string(prop_array, "table", sizeof("table") - 1, field->table, field->table_length TSRMLS_CC);
					mysqlnd_qc_add_to_array_string(prop_array, "orig_table", sizeof("orig_table") - 1, field->org_table, field->org_table_length TSRMLS_CC);
					mysqlnd_qc_add_to_array_string(prop_array, "db", sizeof("db") - 1, field->db, field->db_length TSRMLS_CC);
					mysqlnd_qc_add_to_array_long(prop_array, "max_length", sizeof("max_length") - 1, field->max_length TSRMLS_CC);
					mysqlnd_qc_add_to_array_long(prop_array, "length", sizeof("length") - 1, field->length TSRMLS_CC);
					mysqlnd_qc_add_to_array_long(prop_array, "type", sizeof("type") - 1, field->type TSRMLS_CC);
					add_next_index_zval(meta_array, prop_array);
				}
				mysqlnd_qc_add_to_array_zval(inner_array, "metadata", sizeof("metadata") - 1, meta_array TSRMLS_CC);
			}

			mysqlnd_qc_add_to_array_zval(return_value, str_key, str_key_len - 1, inner_array TSRMLS_CC);
			zend_hash_move_forward_ex(&mysqlnd_qc_qcache.ht, &pos);
		}
	}
	UNLOCK_QCACHE(mysqlnd_qc_qcache);

	return num_entries;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_get_stats()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_get_stats)
{
	zval  *obj;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_get_stats");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_default_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	mysqlnd_qc_handler_default_fill_stats_hash(return_value TSRMLS_CC ZEND_FILE_LINE_CC);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_clear_cache */
enum_func_status
mysqlnd_qc_handler_default_clear_cache(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_handler_default_clear_cache");
	LOCK_QCACHE(mysqlnd_qc_qcache);
	zend_hash_clean(&mysqlnd_qc_qcache.ht);
	UNLOCK_QCACHE(mysqlnd_qc_qcache);
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_clear_cache()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_clear_cache)
{
	zval  *obj;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_clear_cache");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_default_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_BOOL(PASS == mysqlnd_qc_handler_default_clear_cache(TSRMLS_C));
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_init()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_init)
{
	zval  *obj;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_init");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_default_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_TRUE;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_handler_default_shutdown()
 */
static PHP_FUNCTION(mysqlnd_qc_handler_default_shutdown)
{
	zval  *obj;

	DBG_ENTER("zif_mysqlnd_qc_handler_default_shutdown");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &obj, mysqlnd_qc_handler_default_class_entry) == FAILURE) {
		DBG_VOID_RETURN;
	}
	RETVAL_TRUE;
	DBG_VOID_RETURN;
}
/* }}} */


static void mysqlnd_qc_handler_default_minit(TSRMLS_D);
static void mysqlnd_qc_handler_default_mshutdown(TSRMLS_D);

/* {{{ mysqlnd_qc_handler_init */
static void
mysqlnd_qc_handler_default_handler_minit(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_handler_init");
#ifdef ZTS
	mysqlnd_qc_qcache.LOCK_access = tsrm_mutex_alloc();
#endif
	zend_hash_init(&mysqlnd_qc_qcache.ht, 0, NULL, mysqlnd_qc_hash_element_dtor_func, 1);

	mysqlnd_qc_handler_default_minit(TSRMLS_C);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_handler_shutdown */
static void
mysqlnd_qc_handler_default_handler_mshutdown(TSRMLS_D)
{
	zend_hash_destroy(&mysqlnd_qc_qcache.ht);
#ifdef ZTS
	tsrm_mutex_free(mysqlnd_qc_qcache.LOCK_access);
#endif

	mysqlnd_qc_handler_default_mshutdown(TSRMLS_C);
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_std) = {
	MYSQLND_QC_STD_HANDLER_NAME,
	MYSQLND_QC_STD_HANDLER_VERSION_STR,
	mysqlnd_qc_handler_default_get_hash_key,
	mysqlnd_qc_handler_default_query_is_cached,
	mysqlnd_qc_handler_default_find_query_in_cache,
	mysqlnd_qc_handler_default_return_to_cache,
	mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists,
	mysqlnd_qc_handler_default_update_cache_stats,
	mysqlnd_qc_handler_default_fill_stats_hash,
	mysqlnd_qc_handler_default_clear_cache,
	mysqlnd_qc_handler_default_handler_minit,
	mysqlnd_qc_handler_default_handler_mshutdown,
	NULL, /* handler_change_init */
	NULL,  /* handler_change_shutdown */
	NULL  /* handler_change_refresh */
MYSQLND_CLASS_METHODS_END;


/* {{{ property handler_default_read_entries_count */
static int
handler_default_read_entries_count(mysqlnd_qc_handler_object * notused, zval **retval TSRMLS_DC)
{
	DBG_ENTER("handler_default_read_entries_count");
	MAKE_STD_ZVAL(*retval);
	LOCK_QCACHE(mysqlnd_qc_qcache);
	ZVAL_LONG(*retval, zend_hash_num_elements(&mysqlnd_qc_qcache.ht));
	UNLOCK_QCACHE(mysqlnd_qc_qcache);
	DBG_RETURN(SUCCESS);
}
/* }}} */



/* {{{ proto object mysqlnd_qc_handler_default_construct() */
PHP_FUNCTION(mysqlnd_qc_handler_default_construct)
{
	DBG_ENTER("zif_mysqlnd_qc_handler_default_construct");
	DBG_VOID_RETURN;
}
/* }}} */


static HashTable mysqlnd_qc_handler_default_properties;
zend_class_entry * mysqlnd_qc_handler_default_class_entry;

/* {{{ mysqlnd_qc_handler_default_methods[] */
const zend_function_entry mysqlnd_qc_handler_default_methods[] = {
	PHP_FALIAS(MYSQLND_QC_GET_HASH_KEY_METHOD,		mysqlnd_qc_handler_default_get_hash_key, NULL)
	PHP_FALIAS(MYSQLND_QC_FIND_IN_CACHE_METHOD,		mysqlnd_qc_handler_default_find_query_in_cache, NULL)
	PHP_FALIAS(MYSQLND_QC_RETURN_TO_CACHE_METHOD,	mysqlnd_qc_handler_default_return_to_cache, NULL)
	PHP_FALIAS(MYSQLND_QC_ADD_TO_CACHE_METHOD,		mysqlnd_qc_handler_default_add_query_to_cache_if_not_exists, NULL)
	PHP_FALIAS(MYSQLND_QC_UPDATE_CACHE_STATS_METHOD,mysqlnd_qc_handler_default_update_cache_stats, NULL)
	PHP_FALIAS(MYSQLND_QC_FILL_STATS_METHOD,		mysqlnd_qc_handler_default_get_stats, NULL)
	PHP_FALIAS(MYSQLND_QC_CLEAR_CACHE_METHOD,		mysqlnd_qc_handler_default_clear_cache, NULL)
	PHP_FALIAS(MYSQLND_QC_INIT_METHOD,				mysqlnd_qc_handler_default_init, NULL)
	PHP_FALIAS(MYSQLND_QC_SHUTDOWN_METHOD,			mysqlnd_qc_handler_default_shutdown, NULL)
	{NULL, NULL, NULL}
};
/* }}} */

static const mysqlnd_qc_handler_property_entry mysqlnd_qc_handler_default_property_entries[] = {
	{"entries",		sizeof("entries")-1,	handler_default_read_entries_count, NULL},
	{NULL, 0, NULL, NULL}
};

/* should not be const, as it is patched during runtime */

#if PHP_VERSION_ID < 50400
static zend_property_info mysqlnd_qc_handler_default_property_info_entries[] = {
	{ZEND_ACC_PUBLIC, "entries",	sizeof("entries") - 1,		0, NULL, 0, NULL},
	{0,					NULL, 		0,							0, NULL, 0, NULL}
};
#else
static zend_property_info mysqlnd_qc_handler_default_property_info_entries[] = {
	{ZEND_ACC_PUBLIC, "entries",	sizeof("entries") - 1,	-1,	0, NULL, 0, NULL},
	{0,					NULL, 		0,						-1,	0, NULL, 0, NULL}
};

#endif


/* {{{ mysqlnd_qc_handler_default_minit */
static void
mysqlnd_qc_handler_default_minit(TSRMLS_D)
{
	REGISTER_MYSQLND_QC_CLASS("mysqlnd_qc_handler_default", mysqlnd_qc_handler_default_class_entry, mysqlnd_qc_handler_default_methods, NULL, "mysqlnd_qc_handler");
	zend_hash_init(&mysqlnd_qc_handler_default_properties, 0, NULL, NULL, 1);
	MYSQLND_QC_ADD_PROPERTIES(&mysqlnd_qc_handler_default_properties, mysqlnd_qc_handler_default_property_entries);
	MYSQLND_QC_ADD_PROPERTIES_INFO(mysqlnd_qc_handler_default_class_entry, mysqlnd_qc_handler_default_property_info_entries);

	zend_hash_add(&mysqlnd_qc_classes, mysqlnd_qc_handler_default_class_entry->name, mysqlnd_qc_handler_default_class_entry->name_length + 1,
					&mysqlnd_qc_handler_default_properties, sizeof(mysqlnd_qc_handler_default_properties), NULL);
}
/* }}} */


/* {{{ mysqlnd_qc_handler_default_mshutdown */
static void
mysqlnd_qc_handler_default_mshutdown(TSRMLS_D)
{
	zend_hash_destroy(&mysqlnd_qc_handler_default_properties);
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
