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
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_mysqlnd_qc.h"
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_zval_util.h"
#include "mysqlnd_qc_std_handler.h"
#include "mysqlnd_qc_apc_handler.h"
#include "ext/mysqlnd/mysqlnd_alloc.h"
#include "zend_llist.h"

#ifdef ZTS
static MUTEX_T LOCK_qc_apc = NULL;
#define LOCK_QC_APC()		tsrm_mutex_lock(LOCK_qc_apc)
#define UNLOCK_QC_APC()		tsrm_mutex_unlock(LOCK_qc_apc)
#else
#define LOCK_QC_APC()
#define UNLOCK_QC_APC()
#endif

#define QC_APC_SLAM_DEFENSE 1

/* for SG() */
#include "main/SAPI.h"

#include "ext/apc/php_apc.h"
#include "ext/apc/apc_zend.h"

#define SET_CURRENT_TIME(t) 						\
{													\
	if (MYSQLND_QC_G(use_request_time)) { 			\
		if (APCG(use_request_time)) { 				\
			(t) = SG(global_request_time); 			\
		} else { 									\
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s MYSQLND_QC uses sapi_time (mysqlnd_qc.use_request_time=1), APC does not (apc.use_request_time=0). TTL invalidation may not work properly. Make sure the settings are identical ", MYSQLND_QC_ERROR_PREFIX); 			\
			(t) = SG(global_request_time); 			\
		} 											\
	} else { 										\
		if (APCG(use_request_time)) { 				\
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s MYSQLND_QC does not use sapi_time (mysqlnd_qc.use_request_time=0), APC does (apc.use_request_time=1). TTL invalidation may not work properly. Make sure the settings are identical ", MYSQLND_QC_ERROR_PREFIX); 			\
			(t) = time(NULL); 						\
		} else { 									\
			(t) = time(NULL); 						\
		} 											\
	} 												\
}

typedef struct st_mysqlnd_qc_apc_identifier {
	char *identifier;
    int identifier_len;
} MYSQLND_QC_APC_IDENTIFIER;


/* {{{ mysqlnd_qc_apc_find_cache_entries */
static zend_bool mysqlnd_qc_apc_find_cache_entries(zend_llist * entries TSRMLS_DC)
{
	zend_bool ret = TRUE;
	char * prefix;
	int prefix_len;
	slot_t* p;
	int i;
	apc_context_t ctxt = {0,};
	MYSQLND_QC_APC_IDENTIFIER * id;

	DBG_ENTER("mysqlnd_qc_apc_find_cache_entries");

	prefix = (char*)estrndup(MYSQLND_QC_G(apc_prefix), strlen(MYSQLND_QC_G(apc_prefix)));
	prefix_len = strlen(prefix);

	if (!apc_user_cache) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s APC user cache does not exist", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(FALSE);
	}

	LOCK_QC_APC();

	ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
    if (!ctxt.pool) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for APC context pool", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(FALSE);
	}
	ctxt.copy = APC_COPY_OUT_USER;
	ctxt.force_update = 0;

	CACHE_RDLOCK(apc_user_cache);
	for (i = 0; i < apc_user_cache->num_slots; i++) {
		p = apc_user_cache->slots[i];
		for (; p != NULL; p = p->next) {
			if (p->value->type != APC_CACHE_ENTRY_USER) {
				continue;
			}
			if (p->value->data.user.info_len < prefix_len || memcmp(prefix, p->value->data.user.info, prefix_len)) {
				continue;
			}
			id = mnd_pecalloc(1, sizeof(MYSQLND_QC_APC_IDENTIFIER), 0);
			id->identifier = mnd_pestrndup(p->value->data.user.info, p->value->data.user.info_len, 0);
			id->identifier_len = p->value->data.user.info_len;
			zend_llist_add_element(entries, &id);
		}
	}

	CACHE_RDUNLOCK(apc_user_cache);
	apc_pool_destroy(ctxt.pool TSRMLS_CC);
	efree(prefix);

	UNLOCK_QC_APC();

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_apc_cache_entries_list_dtor */
static void
mysqlnd_apc_cache_entries_list_dtor(void * pDest)
{
	MYSQLND_QC_APC_IDENTIFIER * id = *(MYSQLND_QC_APC_IDENTIFIER **) pDest;
	TSRMLS_FETCH();
	if (id) {
		if (id->identifier) {
			mnd_pefree(id->identifier, 0);
		}
		mnd_pefree(id, 0);
	}
}
/* }}} */


int _apc_store(char *strkey, int strkey_len, const zval *val, const unsigned int ttl, const int exclusive TSRMLS_DC);

/* {{{ mysqlnd_qc_apc_get_hash_key */
char *
mysqlnd_qc_apc_get_hash_key(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	char * query_hash_key = NULL;
	smart_str * stripped_query = mysqlnd_qc_query_strip_comments_and_fix_ws(query, query_len TSRMLS_CC);

	DBG_ENTER("mysqlnd_qc_apc_get_hash_key");

	if (stripped_query) {
		if (server_id) {
			*query_hash_key_len = spprintf(&query_hash_key, 0, "%s%s|%s", MYSQLND_QC_G(apc_prefix), server_id, stripped_query->c);
		} else {
			*query_hash_key_len = spprintf(&query_hash_key, 0, "%s%s-%d-%d-%s-%s|%s", MYSQLND_QC_G(apc_prefix), conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", stripped_query->c);
		}
		smart_str_free_ex(stripped_query, 0);
		efree(stripped_query);
	} else {
		if (server_id) {
			*query_hash_key_len = spprintf(&query_hash_key, 0, "%s%s|%s", MYSQLND_QC_G(apc_prefix), server_id, query);
		} else {
			*query_hash_key_len = spprintf(&query_hash_key, 0, "%s%s-%d-%d-%s-%s|%s", MYSQLND_QC_G(apc_prefix), conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", query);
		}
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

/* {{{ mysqlnd_qc_apc_query_is_cached */
static zend_bool
mysqlnd_qc_apc_query_is_cached(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC)
{
	zend_bool ret = FALSE;
	char * query_hash_key = NULL;
	size_t query_hash_key_len;
	apc_context_t ctxt = {0,};
	apc_cache_entry_t* entry;
	zval * zv_cache_entry;
	zval ** zv_valid_until;
	zval ** zv_in_refresh_until;
	long ttl;

	DBG_ENTER("mysqlnd_qc_apc_query_is_cached");

	query_hash_key = mysqlnd_qc_apc_get_hash_key(conn, query, query_len, &query_hash_key_len, server_id, server_id_len, 0 TSRMLS_CC);
	if (query_hash_key_len) {
		time_t now;
		ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
		if (!ctxt.pool) {
			DBG_RETURN(ret);
		}
		ctxt.copy = APC_COPY_OUT_USER;
		ctxt.force_update = 0;

		SET_CURRENT_TIME(now);

		LOCK_QC_APC();
		entry = apc_cache_user_find(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1, now TSRMLS_CC);
		if (entry) {

			MAKE_STD_ZVAL(zv_cache_entry);
			apc_cache_fetch_zval(zv_cache_entry, entry->data.user.val, &ctxt TSRMLS_CC);
			apc_cache_release(apc_user_cache, entry TSRMLS_CC);

			if ((FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "valid_until", sizeof("valid_until"), (void**) &zv_valid_until)) ||
				(IS_LONG != Z_TYPE_PP(zv_valid_until))
				)
			{
				ret = FALSE;
				goto query_is_cached_exit;
			}
			ttl = Z_LVAL_PP(zv_valid_until) - (long)now;
			if (ttl > 0) {
				ret = TRUE;
				goto query_is_cached_exit;
			}

#ifdef QC_APC_SLAM_DEFENSE
			if ((FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "in_refresh_until", sizeof("in_refresh_until"), (void**) &zv_in_refresh_until)) ||
				(IS_LONG != Z_TYPE_PP(zv_in_refresh_until)))
			{
				ret = FALSE;
				goto query_is_cached_exit;
			}
			if (MYSQLND_QC_G(slam_defense)) {
				if (Z_LVAL_PP(zv_in_refresh_until) == 0) {
					/* Set slam defence TTL of the record */
					Z_LVAL_PP(zv_in_refresh_until) = MYSQLND_QC_G(slam_defense_ttl) + (long)now;
					ttl = (long)MYSQLND_QC_G(slam_defense_ttl);
					_apc_store((char *)query_hash_key, query_hash_key_len + 1, zv_cache_entry, ttl, 0 TSRMLS_CC);
				} else {
					ttl = (Z_LVAL_PP(zv_in_refresh_until) - (long)now);
				}
				ret = (ttl > 0) ? TRUE : FALSE;
			}
#endif
		}
	}

query_is_cached_exit:
	UNLOCK_QC_APC();
	apc_pool_destroy(ctxt.pool TSRMLS_CC);
	if (entry)
		zval_ptr_dtor(&zv_cache_entry);
	if (query_hash_key_len)
		efree(query_hash_key);

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ mysqlnd_qc_apc_find_query_in_cache */
static smart_str *
mysqlnd_qc_apc_find_query_in_cache(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC)
{
	time_t now;
	zval * zv_cache_entry;
	zval ** zv_valid_until;
#ifdef QC_APC_SLAM_DEFENSE
	zval ** zv_in_refresh_until;
	zend_bool slam_stale_refresh = FALSE;
#endif
	long ttl;
	zval ** zv_cache_hits;
	zval ** zv_recorded_data;
	apc_context_t ctxt = {0,};
	apc_cache_entry_t* entry;
	smart_str * cached_query = NULL;
	zend_bool bail = FALSE;
	char * bail_msg = NULL;


	DBG_ENTER("mysqlnd_qc_apc_find_query_in_cache");

	ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
	if (!ctxt.pool) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for APC pool", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(cached_query);
	}
	ctxt.copy = APC_COPY_OUT_USER;
	ctxt.force_update = 0;

	SET_CURRENT_TIME(now);

	LOCK_QC_APC();
	entry = apc_cache_user_find(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1, now TSRMLS_CC);
	if (entry) {
		DBG_INF_FMT("entry exists");

		MAKE_STD_ZVAL(zv_cache_entry);
		apc_cache_fetch_zval(zv_cache_entry, entry->data.user.val, &ctxt TSRMLS_CC);
		apc_cache_release(apc_user_cache, entry TSRMLS_CC);

		if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "valid_until", sizeof("valid_until"), (void**) &zv_valid_until)) {
			bail_msg = "valid_until not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto find_exit;
		}

		if (IS_LONG != Z_TYPE_PP(zv_valid_until)) {
			bail_msg = "unexpected type valid_until";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto find_exit;
		}
		ttl = Z_LVAL_PP(zv_valid_until) - (long)now;

#ifdef QC_APC_SLAM_DEFENSE
		if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "in_refresh_until", sizeof("in_refresh_until"), (void**) &zv_in_refresh_until)) {
			bail_msg = "in_refresh_until not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto find_exit;
		}

		if (IS_LONG != Z_TYPE_PP(zv_in_refresh_until)) {
			bail_msg = "unexpected type in_refresh_until";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto find_exit;
		}

		if (MYSQLND_QC_G(slam_defense)) {

			if (ttl >= 0) {
				/* TTL is for APC - APC shall keep the record */
				ttl = 0;
				DBG_INF("cache hit");
			} else if (Z_LVAL_PP(zv_in_refresh_until) == 0) {
				Z_LVAL_PP(zv_in_refresh_until) = MYSQLND_QC_G(slam_defense_ttl) + (long)now;
				/* Set APCs' TTL to slam_defense_ttl */
				ttl = (long)MYSQLND_QC_G(slam_defense_ttl);
				slam_stale_refresh = TRUE;
				MYSQLND_QC_INC_STATISTIC(QC_STAT_SLAM_STALE_REFRESH);
				DBG_INF("slam refresh");
			} else {
				/* slam defense in action */
				ttl = Z_LVAL_PP(zv_in_refresh_until) - (long)now;
				MYSQLND_QC_INC_STATISTIC(QC_STAT_SLAM_STALE_HIT);
				DBG_INF("slam hit");
			}
		}
#endif
		if (ttl < 0) {
			/* cache entry has expired - should not happen */
			DBG_INF("cache entry has expired while processing find");
			goto find_exit;
		}

		if (FALSE == slam_stale_refresh) {
			if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "recorded_data", sizeof("recorded_data"), (void**) &zv_recorded_data)) {
				cached_query = mnd_calloc(1, sizeof(smart_str));
				smart_str_appendl_ex(cached_query, Z_STRVAL_PP(zv_recorded_data), Z_STRLEN_PP(zv_recorded_data) + 1, PERSISTENT_STR);
				DBG_INF("fetched recorded data");
			} else {
				bail_msg = "recorded_data not found";
				DBG_ERR(bail_msg);
				bail = TRUE;
				goto find_exit;
			}
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "cache_hits", sizeof("cache_hits"), (void**) &zv_cache_hits)) {
			if (IS_LONG != Z_TYPE_PP(zv_cache_hits)) {
				bail_msg = "unexpected type cache_hits";
				DBG_ERR(bail_msg);
				bail = TRUE;
				goto find_exit;
			}
			Z_LVAL_PP(zv_cache_hits) = Z_LVAL_PP(zv_cache_hits) + 1;
		} else {
			bail_msg = "cache_hits not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto find_exit;
		}


		/* APC will copy our zvals */
		apc_cache_user_delete(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1 TSRMLS_CC);
		if (!_apc_store((char *)query_hash_key, query_hash_key_len + 1, zv_cache_entry, ttl, 0 TSRMLS_CC)) {
			bail_msg = "apc_store failed";
			DBG_INF(bail_msg);
			bail = TRUE;
			goto find_exit;
		}
	} else {
		DBG_INF("cache miss");
	}

find_exit:
	UNLOCK_QC_APC();
	if (TRUE == bail) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Cache entry '%s' seems corrupted (details: %s)", MYSQLND_QC_ERROR_PREFIX, query_hash_key, (bail_msg) ? bail_msg : "n/a");
		if (cached_query) {
			smart_str_free_ex(cached_query, PERSISTENT_STR);
			mnd_free(cached_query);
			cached_query = NULL;
		}

	}
	apc_pool_destroy(ctxt.pool TSRMLS_CC);

	if (entry)
		zval_ptr_dtor(&zv_cache_entry);

	DBG_INF_FMT("cached_query=%p", cached_query);
	DBG_RETURN(cached_query);
}
/* }}} */


/* {{{ mysqlnd_qc_apc_return_to_cache */
static void
mysqlnd_qc_apc_return_to_cache(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_apc_return_to_cache");
	/* LEAK - what if replay does not work and we never get here - same leak in user handler code */
	smart_str_free_ex(cached_query, PERSISTENT_STR);
	mnd_free(cached_query);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_apc_add_query_to_cache_if_not_exists */
static enum_func_status
mysqlnd_qc_apc_add_query_to_cache_if_not_exists(const MYSQLND_RES * const result,
												const char * query_hash_key, size_t query_hash_key_len,
												smart_str * recorded_data, uint TTL, uint64_t run_time,
												uint64_t store_time, uint64_t row_count TSRMLS_DC)
{

	time_t now;
	enum_func_status ret = FAIL;
	apc_context_t ctxt = {0,};
	apc_cache_entry_t* entry;
	unsigned int apc_ttl = (unsigned int)TTL;
	zend_bool bail = FALSE;
	char * bail_msg = NULL;

	DBG_ENTER("mysqlnd_qc_apc_add_query_to_cache_if_not_exists");

	ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
	if (!ctxt.pool) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for APC pool", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(ret);
	}
	ctxt.copy = APC_COPY_OUT_USER;
	ctxt.force_update = 0;

	SET_CURRENT_TIME(now);

	LOCK_QC_APC();
	entry = apc_cache_user_find(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1, now TSRMLS_CC);
	if (entry) {

#ifdef QC_APC_SLAM_DEFENSE
		if (MYSQLND_QC_G(slam_defense)) {

			zval * zv_cache_entry;
			zval ** zv_valid_until;
			zval ** zv_in_refresh_until;

			ret = FAIL;

			MAKE_STD_ZVAL(zv_cache_entry);
			apc_cache_fetch_zval(zv_cache_entry, entry->data.user.val, &ctxt TSRMLS_CC);
			apc_cache_release(apc_user_cache, entry TSRMLS_CC);


			if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "valid_until", sizeof("valid_until"), (void**) &zv_valid_until)) {
				bail_msg = "valid_until not found";
				DBG_ERR(bail_msg);
				bail = TRUE;
				zval_ptr_dtor(&zv_cache_entry);
				goto add_if_not_exists_exit;
			}

			if (IS_LONG != Z_TYPE_PP(zv_valid_until)) {
				bail_msg = "unexpected type valid_until";
				DBG_ERR(bail_msg);
				bail = TRUE;
				zval_ptr_dtor(&zv_cache_entry);
				goto add_if_not_exists_exit;
			}

			if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "in_refresh_until", sizeof("in_refresh_until"), (void**) &zv_in_refresh_until)) {
				bail_msg = "in_refresh_until not found";
				DBG_ERR(bail_msg);
				bail = TRUE;
				zval_ptr_dtor(&zv_cache_entry);
				goto add_if_not_exists_exit;
			}

			if (IS_LONG != Z_TYPE_PP(zv_in_refresh_until)) {
				bail_msg = "unexpected type in_refresh_until";
				DBG_ERR(bail_msg);
				bail = TRUE;
				zval_ptr_dtor(&zv_cache_entry);
				goto add_if_not_exists_exit;
			}

			Z_LVAL_PP(zv_in_refresh_until) = 0;
			Z_LVAL_PP(zv_valid_until) = (long)TTL + (long)now;
			DBG_INF("slam stale update");

			if (_apc_store((char *)query_hash_key, query_hash_key_len + 1, zv_cache_entry, (unsigned int)0, 0 TSRMLS_CC)) {
				smart_str_free_ex(recorded_data, PERSISTENT_STR);
				mnd_free(recorded_data);
				ret = PASS;
			} else {
				bail_msg = "storing updated cached entry has failed";
				DBG_ERR(bail_msg);
				bail = TRUE;
				zval_ptr_dtor(&zv_cache_entry);
				goto add_if_not_exists_exit;
			}

			apc_cache_release(apc_user_cache, entry TSRMLS_CC);
			zval_ptr_dtor(&zv_cache_entry);
		} else {
			ret = PASS;
			apc_cache_release(apc_user_cache, entry TSRMLS_CC);
		}
#else
		ret = PASS;
		apc_cache_release(apc_user_cache, entry);
#endif
	} else {
		zval zv_cache_entry;
		INIT_ZVAL(zv_cache_entry);
		array_init(&zv_cache_entry);

		mysqlnd_qc_add_to_array_string(&zv_cache_entry, "recorded_data", sizeof("recorded_data") - 1, recorded_data->c, recorded_data->len TSRMLS_CC);

		mysqlnd_qc_add_to_array_long(&zv_cache_entry, "rows", sizeof("rows") - 1, (long)row_count TSRMLS_CC);

		mysqlnd_qc_add_to_array_long(&zv_cache_entry, "cache_hits", sizeof("cache_hits") - 1, (long)0 TSRMLS_CC);

		mysqlnd_qc_add_to_array_long(&zv_cache_entry, "valid_until", sizeof("valid_until") - 1, (long)now + (long)TTL TSRMLS_CC);

		mysqlnd_qc_add_to_array_null(&zv_cache_entry, "min_run_time", sizeof("min_run_time") - 1 TSRMLS_CC);

		mysqlnd_qc_add_to_array_null(&zv_cache_entry, "max_run_time", sizeof("max_run_time") - 1 TSRMLS_CC);

		mysqlnd_qc_add_to_array_null(&zv_cache_entry, "avg_run_time", sizeof("avg_run_time") - 1 TSRMLS_CC);

		mysqlnd_qc_add_to_array_long(&zv_cache_entry, "run_time", sizeof("run_time") - 1, run_time TSRMLS_CC);

		mysqlnd_qc_add_to_array_null(&zv_cache_entry, "min_store_time", sizeof("min_store_time") - 1 TSRMLS_CC);

		mysqlnd_qc_add_to_array_null(&zv_cache_entry, "max_store_time", sizeof("max_store_time") - 1 TSRMLS_CC);

		mysqlnd_qc_add_to_array_null(&zv_cache_entry, "avg_store_time", sizeof("avg_store_time") - 1 TSRMLS_CC);

		mysqlnd_qc_add_to_array_long(&zv_cache_entry, "store_time", sizeof("store_time") - 1, store_time TSRMLS_CC);

#ifdef QC_APC_SLAM_DEFENSE
		mysqlnd_qc_add_to_array_long(&zv_cache_entry, "in_refresh_until", sizeof("in_refresh_until") - 1, 0 TSRMLS_CC);
		if (MYSQLND_QC_G(slam_defense)) {
			apc_ttl = 0;
		}
#endif
		DBG_INF_FMT("adding new entry, ttl = %u", apc_ttl);
		apc_cache_user_delete(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1 TSRMLS_CC);
		if (_apc_store((char *)query_hash_key, query_hash_key_len + 1, &zv_cache_entry, apc_ttl, 0 TSRMLS_CC)) {
			smart_str_free_ex(recorded_data, PERSISTENT_STR);
			mnd_free(recorded_data);
			ret = PASS;
		}

		zval_dtor(&zv_cache_entry);

	}

add_if_not_exists_exit:
	if (TRUE == bail) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Cache entry '%s' seems corrupted (details: %s)", MYSQLND_QC_ERROR_PREFIX, query_hash_key, (bail_msg) ? bail_msg : NULL);
	}

	apc_pool_destroy(ctxt.pool TSRMLS_CC);
	UNLOCK_QC_APC();

	DBG_INF_FMT("ret=%s", ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_apc_update_cache_stats */
static void
mysqlnd_qc_apc_update_cache_stats(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC)
{
	apc_context_t ctxt = {0,};
	apc_cache_entry_t* entry;
	time_t now;
	long ttl;
	zval * zv_cache_entry;
	zval ** zv_valid_until;
#ifdef QC_APC_SLAM_DEFENSE
	zval ** zv_in_refresh_until;
#endif
	zval ** zv_cache_hits;
	zval ** zv_min_run_time, ** zv_max_run_time, ** zv_avg_run_time;
	zval ** zv_min_store_time, ** zv_max_store_time, ** zv_avg_store_time;
	zend_bool bail = FALSE;
	char * bail_msg = NULL;

	DBG_ENTER("mysqlnd_qc_apc_update_cache_stats");

	ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
	if (!ctxt.pool) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for APC pool", MYSQLND_QC_ERROR_PREFIX);
		DBG_VOID_RETURN;
	}
	ctxt.copy = APC_COPY_OUT_USER;
	ctxt.force_update = 0;

	SET_CURRENT_TIME(now);

	LOCK_QC_APC();
	entry = apc_cache_user_find(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1, now TSRMLS_CC);
	if (entry) {
		MAKE_STD_ZVAL(zv_cache_entry);
		apc_cache_fetch_zval(zv_cache_entry, entry->data.user.val, &ctxt TSRMLS_CC);
		apc_cache_release(apc_user_cache, entry TSRMLS_CC);

		if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "valid_until", sizeof("valid_until"), (void**) &zv_valid_until)) {
			bail_msg = "valid_until not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (Z_TYPE_PP(zv_valid_until) != IS_LONG) {
			bail_msg = "unexpected type valid_until";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		ttl = Z_LVAL_PP(zv_valid_until) - (long)now;

#ifdef QC_APC_SLAM_DEFENSE
		if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "in_refresh_until", sizeof("in_refresh_until"), (void**) &zv_in_refresh_until)) {
			bail_msg = "in_refresh_until not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (IS_LONG != Z_TYPE_PP(zv_in_refresh_until)) {
			bail_msg = "unexpected type in_refresh_until";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (MYSQLND_QC_G(slam_defense)) {
			if (ttl >= 0) {
				/* TTL is for APC - APC shall keep the record */
				ttl = 0;
				DBG_INF("cache hit");
			} else if (Z_LVAL_PP(zv_in_refresh_until) == 0) {
				bail_msg = "in_refresh_until is zero";
				DBG_INF("cache logic must be borked, in_refresh_until is zero");
				goto update_stats_exit;
			} else {
				/* slam defense in action */
				ttl = Z_LVAL_PP(zv_in_refresh_until) - (long)now;
				MYSQLND_QC_INC_STATISTIC(QC_STAT_SLAM_STALE_HIT);
				DBG_INF("slam stale hit");
			}
		}
#endif

		if (ttl < 0) {
			/* cache entry has expired - should not happen */
			bail_msg = "expired while processing";
			DBG_INF("cache entry has expired while processing find");
			goto update_stats_exit;
		}

		if (FAILURE == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "cache_hits", sizeof("cache_hits"), (void**) &zv_cache_hits)) {
			bail_msg = "cache_hits not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "min_run_time", sizeof("min_run_time"), (void**) &zv_min_run_time)) {
			switch (Z_TYPE_PP(zv_min_run_time)) {
				case IS_LONG:
					if (0 == Z_LVAL_PP(zv_min_run_time)) {
						Z_LVAL_PP(zv_min_run_time) = run_time;
					} else if (Z_LVAL_PP(zv_min_run_time) > run_time) {
						Z_LVAL_PP(zv_min_run_time) = run_time;
					}
					break;
				case IS_NULL:
					Z_TYPE_PP(zv_min_run_time) = IS_LONG;
					Z_LVAL_PP(zv_min_run_time) = run_time;
					break;
				default:
					bail_msg = "unexpected type min_run_time";
					DBG_ERR(bail_msg);
					bail = TRUE;
					goto update_stats_exit;
					break;
			}
		} else {
			bail_msg = "min_run_time not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "max_run_time", sizeof("max_run_time"), (void**) &zv_max_run_time)) {
			switch (Z_TYPE_PP(zv_max_run_time)) {
				case IS_LONG:
					if (0 == Z_LVAL_PP(zv_max_run_time)) {
						Z_LVAL_PP(zv_max_run_time) = run_time;
					} else if (Z_LVAL_PP(zv_max_run_time) < run_time) {
						Z_LVAL_PP(zv_max_run_time) = run_time;
					}
					break;
				case IS_NULL:
					Z_TYPE_PP(zv_max_run_time) = IS_LONG;
					Z_LVAL_PP(zv_max_run_time) = run_time;
					break;
				default:
					bail_msg = "unexpected type max_run_time";
					DBG_ERR(bail_msg);
					bail = TRUE;
					goto update_stats_exit;
					break;
			}
		} else {
			bail_msg = "max_run_time not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "avg_run_time", sizeof("avg_run_time"), (void**) &zv_avg_run_time)) {
			switch (Z_TYPE_PP(zv_avg_run_time)) {
				case IS_LONG:
					if (0 == Z_LVAL_PP(zv_avg_run_time)) {
						Z_LVAL_PP(zv_avg_run_time) = run_time;
					} else {
						Z_LVAL_PP(zv_avg_run_time) = ((Z_LVAL_PP(zv_cache_hits) * Z_LVAL_PP(zv_avg_run_time)) + run_time) / (Z_LVAL_PP(zv_cache_hits) + 1);
					}
					break;
				case IS_NULL:
					Z_TYPE_PP(zv_avg_run_time) = IS_LONG;
					Z_LVAL_PP(zv_avg_run_time) = run_time;
					break;
				default:
					bail_msg = "unexpected type avg_run_time";
					DBG_ERR(bail_msg);
					bail = TRUE;
					goto update_stats_exit;
					break;
			}
		} else {
			bail_msg = "avg_run_time not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "min_store_time", sizeof("min_store_time"), (void**) &zv_min_store_time)) {
			switch (Z_TYPE_PP(zv_min_store_time)) {
				case IS_LONG:
					if (0 == Z_LVAL_PP(zv_min_store_time)) {
						Z_LVAL_PP(zv_min_store_time) = store_time;
					} else if (Z_LVAL_PP(zv_min_store_time) > store_time) {
						Z_LVAL_PP(zv_min_store_time) = store_time;
					}
					break;
				case IS_NULL:
					Z_TYPE_PP(zv_min_store_time) = IS_LONG;
					Z_LVAL_PP(zv_min_store_time) = store_time;
					break;
				default:
					bail_msg = "unexpected type min_store_time";
					DBG_ERR(bail_msg);
					bail = TRUE;
					goto update_stats_exit;
					break;
			}
		} else {
			bail_msg = "min_store_time not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "max_store_time", sizeof("max_store_time"), (void**) &zv_max_store_time)) {
			switch (Z_TYPE_PP(zv_max_store_time)) {
				case IS_LONG:
					if (0 == Z_LVAL_PP(zv_max_store_time)) {
						Z_LVAL_PP(zv_max_store_time) = store_time;
					} else if (Z_LVAL_PP(zv_max_store_time) < store_time) {
						Z_LVAL_PP(zv_max_store_time) = store_time;
					}
					break;
				case IS_NULL:
					Z_TYPE_PP(zv_max_store_time) = IS_LONG;
					Z_LVAL_PP(zv_max_store_time) = store_time;
					break;
				default:
					bail_msg = "unexpected type max_store_time";
					DBG_ERR(bail_msg);
					bail = TRUE;
					goto update_stats_exit;
					break;
			}
		} else {
			bail_msg = "max_store_time not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "avg_store_time", sizeof("avg_store_time"), (void**) &zv_avg_store_time)) {
			switch (Z_TYPE_PP(zv_avg_store_time)) {
				case IS_LONG:
					if (0 == Z_LVAL_PP(zv_avg_store_time)) {
						Z_LVAL_PP(zv_avg_store_time) = store_time;
					} else {
						Z_LVAL_PP(zv_avg_store_time) = ((Z_LVAL_PP(zv_cache_hits) * Z_LVAL_PP(zv_avg_store_time)) + store_time) / (Z_LVAL_PP(zv_cache_hits) + 1);
					}
					break;
				case IS_NULL:
					Z_TYPE_PP(zv_avg_store_time) = IS_LONG;
					Z_LVAL_PP(zv_avg_store_time) = store_time;
					break;
				default:
					bail_msg = "unexpected type avg_store_time";
					DBG_ERR(bail_msg);
					bail = TRUE;
					goto update_stats_exit;
					break;
			}
		} else {
			bail_msg = "avg_store_time not found";
			DBG_ERR(bail_msg);
			bail = TRUE;
			goto update_stats_exit;
		}

		apc_cache_user_delete(apc_user_cache, (char *)query_hash_key, query_hash_key_len + 1 TSRMLS_CC);
		if (!_apc_store((char *)query_hash_key, query_hash_key_len + 1, zv_cache_entry, (unsigned int)ttl, 0 TSRMLS_CC)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Failed to update statistics of cache entry '%s' ", MYSQLND_QC_ERROR_PREFIX, query_hash_key);
			bail = TRUE;
		}
	}

update_stats_exit:
	UNLOCK_QC_APC();
	if (entry)
		zval_ptr_dtor(&zv_cache_entry);
	if (TRUE == bail) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Cache entry '%s' seems corrupted (details: %s)", MYSQLND_QC_ERROR_PREFIX, query_hash_key, (bail_msg) ? bail_msg : NULL);
	}
	apc_pool_destroy(ctxt.pool TSRMLS_CC);

	DBG_VOID_RETURN;
}
/* }}} */

#define QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, str_key) \
{ \
	zval ** zv_target; \
	if (SUCCESS == zend_hash_find(Z_ARRVAL_P((zv_cache_entry)), (str_key), sizeof((str_key)), (void**) &zv_target)) { \
		zval * tmp; \
		MAKE_STD_ZVAL(tmp); \
		if (Z_TYPE_PP(zv_target) == IS_NULL) { \
			ZVAL_NULL(tmp); \
		} else { \
			ZVAL_LONG(tmp, Z_LVAL_PP(zv_target)); \
		} \
		mysqlnd_qc_add_to_array_zval((zv_stats), (str_key), sizeof((str_key)) - 1, tmp TSRMLS_CC); \
	} else { \
		snprintf(bail_msg, sizeof(bail_msg) - 1, "%s not found", (str_key));  \
		DBG_ERR(bail_msg); \
		bail = TRUE; \
		zval_ptr_dtor(&(zv_stats)); \
		zval_ptr_dtor(&(zv_cache_entry)); \
		goto fill_stats_continue; \
	} \
}

/* {{{ mysqlnd_qc_apc_fill_stats_hash */
static long
mysqlnd_qc_apc_fill_stats_hash(zval *return_value TSRMLS_DC ZEND_FILE_LINE_DC)
{
	long num_entries = 0;
	time_t now;
	zend_bool bail = FALSE;
	char bail_msg[256] = "";
	apc_context_t ctxt = {0,};
	zend_llist * entries;
	zend_llist_position	pos;
	MYSQLND_QC_APC_IDENTIFIER * id, ** id_pp;
	apc_cache_entry_t* entry;

	DBG_ENTER("mysqlnd_qc_apc_fill_stats_hash");

	entries = mnd_pemalloc(sizeof(zend_llist), 0);
	if (!entries) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for temporary list of cache entry names", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(FAIL);
	}
	zend_llist_init(entries, sizeof(char *), (llist_dtor_func_t) mysqlnd_apc_cache_entries_list_dtor /*dtor*/, 0);
	if (FALSE == mysqlnd_qc_apc_find_cache_entries(entries TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Failed to create list of cache entry names", MYSQLND_QC_ERROR_PREFIX);
		zend_llist_clean(entries);
		mnd_pefree(entries, 0);
		DBG_RETURN(FAIL);
	}

	array_init(return_value);

	SET_CURRENT_TIME(now);
	LOCK_QC_APC();

	ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
    if (!ctxt.pool) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for APC context pool", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(num_entries);
	}
	ctxt.copy = APC_COPY_OUT_USER;
	ctxt.force_update = 0;

	for (id_pp = (MYSQLND_QC_APC_IDENTIFIER **) zend_llist_get_first_ex(entries, &pos);
			 id_pp && (id = *id_pp);
			 id_pp = (MYSQLND_QC_APC_IDENTIFIER **) zend_llist_get_next_ex(entries, &pos))
	{
		entry = apc_cache_user_find(apc_user_cache, id->identifier, id->identifier_len, now TSRMLS_CC);
		if (entry) {
			zval * zv_cache_entry;
			zval * zv_stats;
			zval * zv_inner_array;

			bail = FALSE;
			MAKE_STD_ZVAL(zv_cache_entry);

			/* deep-copy returned shm zval to emalloc'ed return_value */
			apc_cache_fetch_zval(zv_cache_entry, entry->data.user.val, &ctxt TSRMLS_CC);
			apc_cache_release(apc_user_cache, entry TSRMLS_CC);

			MAKE_STD_ZVAL(zv_stats);
			array_init(zv_stats);

			{
				zval ** zv_target;
				if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "rows", sizeof("rows"), (void**) &zv_target)) {
					zval * tmp;
					MAKE_STD_ZVAL(tmp);
					ZVAL_LONG(tmp, Z_LVAL_PP(zv_target));
					mysqlnd_qc_add_to_array_zval(zv_stats, "rows", sizeof("rows") - 1, tmp TSRMLS_CC);
				} else {
					memcpy(bail_msg, "rows not found", sizeof("rows not found"));
					DBG_ERR(bail_msg);
					zval_ptr_dtor(&zv_stats);
					zval_ptr_dtor(&zv_cache_entry);
					bail = TRUE;
					goto fill_stats_continue;
				}
			}

			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "min_run_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "max_run_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "avg_run_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "run_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "min_store_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "max_store_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "avg_store_time");
			QC_APC_FILL_STATS_WITH_LONG(zv_cache_entry, zv_stats, "store_time");

			{
				zval ** zv_target;
				if (SUCCESS == zend_hash_find(Z_ARRVAL_P(zv_cache_entry), "cache_hits", sizeof("cache_hits"), (void**) &zv_target)) {
					zval * tmp;
					MAKE_STD_ZVAL(tmp);
					ZVAL_LONG(tmp, Z_LVAL_PP(zv_target));
					mysqlnd_qc_add_to_array_zval(zv_stats, "cache_hits", sizeof("cache_hits") - 1, tmp TSRMLS_CC);
				} else {
					memcpy(bail_msg, "cache_hits not found", sizeof("cache_hits not found"));
					DBG_ERR(bail_msg);
					zval_ptr_dtor(&zv_stats);
					zval_ptr_dtor(&zv_cache_entry);
					bail = TRUE;
					goto fill_stats_continue;
				}
			}

			MAKE_STD_ZVAL(zv_inner_array);
			array_init(zv_inner_array);
			mysqlnd_qc_add_to_array_zval(zv_inner_array, "statistics", sizeof("statistics") - 1, zv_stats TSRMLS_CC);
			mysqlnd_qc_add_to_array_null(zv_inner_array, "metadata", sizeof("metadata") - 1 TSRMLS_CC);

			mysqlnd_qc_add_to_array_zval(return_value, id->identifier, id->identifier_len - 1, zv_inner_array TSRMLS_CC);
			num_entries++;

			zval_ptr_dtor(&zv_cache_entry);
		}
fill_stats_continue:
		if (TRUE == bail) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Cache entry '%s' seems corrupted (details: %s)", MYSQLND_QC_ERROR_PREFIX, id->identifier, (bail_msg[0]) ? bail_msg : "n/a");
			bail = FALSE;
		}
	}
	CACHE_RDUNLOCK(apc_user_cache);

	apc_pool_destroy(ctxt.pool TSRMLS_CC);
	UNLOCK_QC_APC();

	zend_llist_clean(entries);
	mnd_pefree(entries, 0);

	DBG_RETURN(num_entries);
}
/* }}} */


/* {{{ mysqlnd_qc_std_clear_cache */
enum_func_status
mysqlnd_qc_apc_clear_cache(TSRMLS_D)
{
	zend_bool ret = TRUE;
	apc_context_t ctxt = {0,};
	time_t now;
	zend_llist * entries;
	zend_llist_position	pos;
	MYSQLND_QC_APC_IDENTIFIER * id, ** id_pp;
	apc_cache_entry_t* entry;

	DBG_ENTER("mysqlnd_qc_apc_clear_cache");

	entries = mnd_pemalloc(sizeof(zend_llist), 0);
	if (!entries) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for temporary list of cache entry names", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(FAIL);
	}
	zend_llist_init(entries, sizeof(char *), (llist_dtor_func_t) mysqlnd_apc_cache_entries_list_dtor /*dtor*/, 0);
	if (FALSE == mysqlnd_qc_apc_find_cache_entries(entries TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Failed to create list of cache entry names", MYSQLND_QC_ERROR_PREFIX);
		zend_llist_clean(entries);
		mnd_pefree(entries, 0);
		DBG_RETURN(FAIL);
	}

	SET_CURRENT_TIME(now);
	LOCK_QC_APC();

	ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
    if (!ctxt.pool) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s Unable to allocate memory for APC context pool", MYSQLND_QC_ERROR_PREFIX);
		DBG_RETURN(FAIL);
	}
	ctxt.copy = APC_COPY_OUT_USER;
	ctxt.force_update = 0;

	for (id_pp = (MYSQLND_QC_APC_IDENTIFIER **) zend_llist_get_first_ex(entries, &pos);
			 id_pp && (id = *id_pp);
			 id_pp = (MYSQLND_QC_APC_IDENTIFIER **) zend_llist_get_next_ex(entries, &pos))
	{
		entry = apc_cache_user_find(apc_user_cache, id->identifier, id->identifier_len, now TSRMLS_CC);
		if (entry) {
			if (!apc_cache_user_delete(apc_user_cache, id->identifier, id->identifier_len TSRMLS_CC)) {
				DBG_ERR("apc_cache_user_delete() failed");
				ret = FALSE;
				goto clear_cache_exit;
			}
		}
	}


clear_cache_exit:
	CACHE_RDUNLOCK(apc_user_cache);
	apc_pool_destroy(ctxt.pool TSRMLS_CC);

	UNLOCK_QC_APC();

	zend_llist_clean(entries);
	mnd_pefree(entries, 0);

	DBG_RETURN((ret == TRUE) ? PASS : FAIL);
}


static zend_bool apc_handler_initted = FALSE;

/* {{{ mysqlnd_qc_apc_handler_init */
static void
mysqlnd_qc_apc_handler_minit(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_apc_handler_minit");

	/* There is still a race */
	if (apc_handler_initted) {
		DBG_VOID_RETURN;
	}
	apc_handler_initted = TRUE;
#ifdef ZTS
	if (!LOCK_qc_apc) {
		LOCK_qc_apc = tsrm_mutex_alloc();
	}
#endif
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_apc_handler_mshutdown */
static void
mysqlnd_qc_apc_handler_mshutdown(TSRMLS_D)
{
	DBG_ENTER("mysqlnd_qc_apc_handler_mshutdown");
#ifdef ZTS
	tsrm_mutex_free(LOCK_qc_apc);
#endif
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_apc_handler_change_init */
static enum_func_status
mysqlnd_qc_apc_handler_change_init(TSRMLS_D)
{
	enum_func_status ret = PASS;
	DBG_ENTER("mysqlnd_qc_apc_handler_change_init");

	if (!APCG(enabled)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "APC is disabled (apc.enabled=0), cannot use APC for storage. You must enable APC to use it");
		ret = FAIL;
	}

	if (APCG(slam_defense)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "APC slam defense (apc.slam_defense=1) will clash with fast cache updates. You must disable it to be able to use the APC handler");
		ret = FAIL;
	}

	if (MYSQLND_QC_G(use_request_time) != APCG(use_request_time)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "APC and MYSQLND_QC are configured to use different timers. TTL invalidation may fail. You should use the same settings for mysqlnd_qc.use_request_time and apc.use_request_time");
	}

	DBG_RETURN(ret);
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_apc) = {
	MYSQLND_QC_APC_HANDLER_NAME,
	MYSQLND_QC_APC_HANDLER_VERSION_STR,
	mysqlnd_qc_apc_get_hash_key,
	mysqlnd_qc_apc_query_is_cached,
	mysqlnd_qc_apc_find_query_in_cache,
	mysqlnd_qc_apc_return_to_cache,
	mysqlnd_qc_apc_add_query_to_cache_if_not_exists,
	mysqlnd_qc_apc_update_cache_stats,
	mysqlnd_qc_apc_fill_stats_hash,
	mysqlnd_qc_apc_clear_cache,
	mysqlnd_qc_apc_handler_minit,
	mysqlnd_qc_apc_handler_mshutdown,
	mysqlnd_qc_apc_handler_change_init,
	NULL,  /* handler_change_shutdown */
	NULL  /* handler_change_refresh */
MYSQLND_CLASS_METHODS_END;


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
