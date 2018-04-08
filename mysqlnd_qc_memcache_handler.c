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
#include "mysqlnd_qc_memcache_handler.h"
#include "mysqlnd_qc_std_handler.h"
#include "ext/standard/md5.h"

/* for SG() */
#include "main/SAPI.h"

#define CURRENT_TIME ((MYSQLND_QC_G(use_request_time)) ? SG(global_request_time) : time(NULL))

#ifdef ZTS
#include "TSRM.h"
#endif

#include "ext/standard/base64.h"
#include <libmemcached/memcached.h>


#define MYSQLND_QC_MEMC_HANDLER_MAKE_KEY(md5str, key, key_len) {\
	PHP_MD5_CTX context; \
	unsigned char digest[16]; \
	md5str[0] = '\0'; \
	PHP_MD5Init(&context); \
	PHP_MD5Update(&context, (key), (key_len)); \
	PHP_MD5Final(digest, &context); \
	make_digest_ex((md5str), digest, 16); \
}


/* {{{ mysqlnd_qc_memcache_get_hash_key */
static char *
mysqlnd_qc_memcache_get_hash_key(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, size_t * query_hash_key_len, const char * const server_id, size_t server_id_len, zend_bool persistent TSRMLS_DC)
{
	char * query_hash_key = NULL;
	char * md5_key = emalloc(33);
	size_t key_len;
	smart_str * stripped_query = mysqlnd_qc_query_strip_comments_and_fix_ws(query, query_len TSRMLS_CC);

	DBG_ENTER("mysqlnd_qc_memcache_get_hash_key");

	if (stripped_query) {
		if (server_id) {
			key_len = spprintf(&query_hash_key, 0, "key%s|%s", server_id, stripped_query->c);
		} else {
			key_len = spprintf(&query_hash_key, 0, "key%s%d%d%s%s|%s", conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", stripped_query->c);
		}
		smart_str_free_ex(stripped_query, 0);
		efree(stripped_query);
	} else {
		if (server_id) {
			key_len = spprintf(&query_hash_key, 0, "key%s|%s", server_id, query);
		} else {
			key_len = spprintf(&query_hash_key, 0, "key%s%d%d%s%s|%s", conn->host_info, conn->port, ((conn->charset) ? conn->charset->nr : 0), conn->user, conn->connect_or_select_db? conn->connect_or_select_db:"", query);
		}
	}

	MYSQLND_QC_MEMC_HANDLER_MAKE_KEY(md5_key, query_hash_key, key_len);
	efree(query_hash_key);
	md5_key[32] = '\0';
	query_hash_key = md5_key;
	*query_hash_key_len = 32;

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

/* {{{ mysqlnd_qc_memcache_query_is_cached */
static zend_bool
mysqlnd_qc_memcache_query_is_cached(MYSQLND_CONN_DATA * conn, const char * query, size_t query_len, const char * const server_id, size_t server_id_len TSRMLS_DC)
{
	zend_bool ret = FALSE;
	size_t query_hash_key_len;
	char * query_hash_key;
	uint32_t flags = 0;
	memcached_return status;


	DBG_ENTER("mysqlnd_qc_memcache_query_is_cached");
	query_hash_key = mysqlnd_qc_memcache_get_hash_key(conn, query, query_len, &query_hash_key_len, server_id, server_id_len, 0 TSRMLS_CC);
	if (query_hash_key_len) {
#ifdef HAVE_MEMCACHE_EXISTS
		status = memcached_exists(MYSQLND_QC_G(memc), query_hash_key, query_hash_key_len);
#else
		char *payload = NULL;
		size_t payload_len = 0;
		payload = memcached_get(MYSQLND_QC_G(memc), query_hash_key, query_hash_key_len, &payload_len, &flags, &status);
		if (payload) {
			free(payload);
		}
#endif
		ret = (MEMCACHED_SUCCESS == status) ? TRUE : FALSE;
		efree(query_hash_key);
	}

	DBG_RETURN(ret);
}
/* }}} */

/* {{{ mysqlnd_qc_memcache_find_query_in_cache */
static smart_str *
mysqlnd_qc_memcache_find_query_in_cache(const char * query_hash_key, size_t query_hash_key_len TSRMLS_DC)
{
	smart_str * cached_query = NULL;
	char *payload = NULL;
	size_t payload_len = 0;
	uint32_t flags = 0;
	memcached_return status;
	DBG_ENTER("mysqlnd_qc_memcache_find_query_in_cache");

	payload = memcached_get(MYSQLND_QC_G(memc), query_hash_key, query_hash_key_len, &payload_len, &flags, &status);

	if (status == MEMCACHED_NOTFOUND) {
		DBG_INF("not found");
	} else if (status == MEMCACHED_SUCCESS && payload) {
		int decoded_data_length;
		unsigned char * decoded_data = php_base64_decode_ex((unsigned char*)payload, payload_len, &decoded_data_length, TRUE);
		free(payload);
		cached_query = mnd_calloc(1, sizeof(smart_str));
		smart_str_appendl_ex(cached_query, decoded_data, decoded_data_length, PERSISTENT_STR);
		efree(decoded_data);
	} else if (status != MEMCACHED_SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", memcached_strerror(MYSQLND_QC_G(memc), status));
	}

	DBG_RETURN(cached_query);
}
/* }}} */


/* {{{ mysqlnd_qc_memcache_return_to_cache */
static void
mysqlnd_qc_memcache_return_to_cache(const char * query_hash_key, size_t query_hash_key_len, smart_str * cached_query TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_memcache_return_to_cache");

	/* we allocated it, we need to free it */
	smart_str_free_ex(cached_query, PERSISTENT_STR);
	mnd_free(cached_query);

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_add_query_to_cache_if_not_exists */
static enum_func_status
mysqlnd_qc_memcache_add_query_to_cache_if_not_exists(const MYSQLND_RES * const result, const char * query_hash_key, size_t query_hash_key_len,
													smart_str * recorded_data, uint TTL, uint64_t run_time, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	enum_func_status ret = FAIL;
	uint32_t flags = 0;
	char *payload = NULL;
	size_t payload_len = 0;
	memcached_return status;

	DBG_ENTER("mysqlnd_qc_memcache_add_query_to_cache_if_not_exists");

	payload = memcached_get(MYSQLND_QC_G(memc), query_hash_key, query_hash_key_len, &payload_len, &flags, &status);

	DBG_INF_FMT("payload=%p rc=%d SUCCESS=%d NOTFOUND=%d", payload, status, MEMCACHED_SUCCESS, MEMCACHED_NOTFOUND);
	if (status == MEMCACHED_NOTFOUND) {
		memcached_return rc;
		int base64_data_length = 0;
		unsigned char * base64_data = php_base64_encode((unsigned char*)recorded_data->c, recorded_data->len, &base64_data_length);
		rc = memcached_set(MYSQLND_QC_G(memc), query_hash_key, query_hash_key_len, (char *) base64_data, base64_data_length, (time_t)TTL, flags);
		DBG_INF_FMT("MEMCACHED_NOTFOUND. rc=%d", rc);
		efree(base64_data);
		ret = (rc == MEMCACHED_SUCCESS)? PASS:FAIL;
	} else if (status == MEMCACHED_SUCCESS && payload) {
		free(payload);
		ret = FAIL;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", memcached_strerror(MYSQLND_QC_G(memc), status));
	}

	DBG_INF_FMT("ret=%s", ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */



#define CALC_N_STORE_AVG(current_avg, current_count, new_value) (current_avg) = ((current_avg) * (current_count) + (new_value))/ ((current_count) + 1)

/* {{{ mysqlnd_qc_update_cache_stats */
static void
mysqlnd_qc_memcache_update_cache_stats(const char * query_hash_key, size_t query_hash_key_len, uint64_t run_time, uint64_t store_time TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_memcache_update_cache_stats");
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_memcache_fill_stats_hash */
long
mysqlnd_qc_memcache_fill_stats_hash(zval *return_value TSRMLS_DC ZEND_FILE_LINE_DC)
{
	long num_entries = 0;
	DBG_ENTER("mysqlnd_qc_memcache_fill_stats_hash");

	array_init(return_value);
	return num_entries;
}
/* }}} */


/* {{{ mysqlnd_qc_memcache_handler_change_init */
static enum_func_status
mysqlnd_qc_memcache_handler_change_init(TSRMLS_D)
{
	memcached_server_st * servers;
	memcached_return rc;

	DBG_ENTER("mysqlnd_qc_memcache_handler_minit");
	DBG_INF_FMT("server=%s port=%d",  MYSQLND_QC_G(memc_server), MYSQLND_QC_G(memc_port));
	MYSQLND_QC_G(memc) = memcached_create(NULL);
	if (MYSQLND_QC_G(memc)) {
		servers = memcached_server_list_append(NULL, MYSQLND_QC_G(memc_server), MYSQLND_QC_G(memc_port), &rc);
		DBG_INF_FMT("rc=%d", rc);
		rc = memcached_server_push(MYSQLND_QC_G(memc), servers);
		DBG_INF_FMT("rc=%d", rc);
		if (MEMCACHED_SUCCESS == rc) {
			memcached_server_list_free(servers);
			DBG_RETURN(PASS);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", memcached_strerror(MYSQLND_QC_G(memc), rc));
		}
	}
	DBG_RETURN(FAIL);
}
/* }}} */


/* {{{ mysqlnd_qc_memcache_handler_change_shutdown */
static enum_func_status
mysqlnd_qc_memcache_handler_change_shutdown(TSRMLS_D)
{
	if (MYSQLND_QC_G(memc)) {
		memcached_free(MYSQLND_QC_G(memc));
		MYSQLND_QC_G(memc) = NULL;
	}
	return PASS;
}
/* }}} */


/* {{{ mysqlnd_qc_memcache_handler_change_refresh */
static enum_func_status
mysqlnd_qc_memcache_handler_change_refresh(TSRMLS_D)
{
	return mysqlnd_qc_memcache_handler_change_shutdown(TSRMLS_C) == PASS && mysqlnd_qc_memcache_handler_change_init(TSRMLS_C) == PASS ? PASS:FAIL;
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_memcache) = {
	MYSQLND_QC_MEMC_HANDLER_NAME,
	MYSQLND_QC_MEMC_HANDLER_VERSION_STR,
	mysqlnd_qc_memcache_get_hash_key,
	mysqlnd_qc_memcache_query_is_cached,
	mysqlnd_qc_memcache_find_query_in_cache,
	mysqlnd_qc_memcache_return_to_cache,
	mysqlnd_qc_memcache_add_query_to_cache_if_not_exists,
	mysqlnd_qc_memcache_update_cache_stats,
	mysqlnd_qc_memcache_fill_stats_hash,
	NULL, /* clear_cache */
	NULL, /* minit */
	NULL, /* mshutdown */
	mysqlnd_qc_memcache_handler_change_init, /* handler_change_init */
	mysqlnd_qc_memcache_handler_change_shutdown,  /* handler_change_shutdown */
	mysqlnd_qc_memcache_handler_change_refresh /* handler_change_refresh */
MYSQLND_CLASS_METHODS_END;

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
