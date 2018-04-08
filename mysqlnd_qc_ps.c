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
#include "ext/standard/base64.h"

typedef struct st_mysqlnd_pscache_stmt_data
{
	char *		query;
	size_t		query_len;

	char *		query_hash_key;
	size_t		query_hash_key_len;

	uint 		TTL;
	uint64_t	run_time;
	zend_bool	record_mode;
	zend_bool	use_result;

	char *		server_id;
	size_t		server_id_len;
} MYSQLND_QC_STMT_DATA;

static func_mysqlnd_stmt__prepare orig_mysqlnd_stmt__prepare;
static func_mysqlnd_stmt__execute orig_mysqlnd_stmt__execute;
static func_mysqlnd_stmt__store_result orig_mysqlnd_stmt__store_result;
static func_mysqlnd_stmt__use_result orig_mysqlnd_stmt__use_result;
static func_mysqlnd_stmt__fetch orig_mysqlnd_stmt__fetch;
static func_mysqlnd_stmt__generate_execute_request orig_mysqlnd_stmt__generate_execute_request;
static func_mysqlnd_stmt__dtor orig_mysqlnd_stmt__dtor;
static func_mysqlnd_stmt__free_stmt_content orig_mysqlnd_stmt_free_stmt_content;


/* {{{ mysqlnd_qc_ps_free_stmt_plugin_data */
void
mysqlnd_qc_ps_free_stmt_plugin_data(MYSQLND_QC_STMT_DATA ** stmt_data_pp, zend_bool persistent TSRMLS_DC)
{
	DBG_ENTER("mysqlnd_qc_ps_free_stmt_plugin_data");
	if (stmt_data_pp && *stmt_data_pp) {
		DBG_INF_FMT("statement data %p", *stmt_data_pp);
		if ((*stmt_data_pp)->query) {
			mnd_pefree((*stmt_data_pp)->query, persistent);
		}
		if ((*stmt_data_pp)->query_hash_key) {
			/* hash key comes from the handler and is !!!always!!! emalloc-ed */
			pefree((*stmt_data_pp)->query_hash_key, persistent);
		}
		if ((*stmt_data_pp)->server_id) {
			mnd_pefree((*stmt_data_pp)->server_id, persistent);
		}
		mnd_pefree((*stmt_data_pp), persistent);
		*stmt_data_pp = NULL;
	}
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::prepare */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, prepare)(MYSQLND_STMT * const s, const char * const query, unsigned int query_len TSRMLS_DC)
{
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);
	enum_func_status ret;
	uint TTL = 0;
	char * server_id = NULL;
	size_t server_id_len = 0;
	zend_bool should_cache = FALSE;
	MYSQLND_STMT_DATA * stmt = s->data;
	MYSQLND_QC_CONNECTION_DATA ** conn_data_pp;
	zend_bool persistent = stmt->persistent;
	DBG_ENTER("mysqlnd_qc_ps_stmt::prepare");

	mysqlnd_qc_ps_free_stmt_plugin_data(stmt_data_pp, persistent TSRMLS_CC);

	QC_LOAD_CONN_DATA(conn_data_pp, s->data->conn);
	MYSQLND_QC_NET_RESTORE_ORIGINAL(s->data->conn, conn_data_pp);

	should_cache = mysqlnd_qc_query_is_select(query, query_len, &TTL, &server_id, &server_id_len TSRMLS_CC);

	/* ******************** */
	ret = orig_mysqlnd_stmt__prepare(s, query, query_len TSRMLS_CC);
	/* ******************** */

	if (PASS == ret) {
		DBG_INF("elligible for caching");
		if (should_cache) {
			*stmt_data_pp = mnd_pecalloc(1, sizeof(MYSQLND_QC_STMT_DATA), persistent);
			(*stmt_data_pp)->TTL = TTL;

			(*stmt_data_pp)->query = mnd_pemalloc(query_len + 1, persistent);
			memcpy((*stmt_data_pp)->query, query, query_len + 1);
			(*stmt_data_pp)->query_len = query_len;

			if (server_id) {
				(*stmt_data_pp)->server_id = mnd_pemalloc(server_id_len + 1, persistent);
				memcpy((*stmt_data_pp)->server_id, server_id, server_id_len + 1);
				(*stmt_data_pp)->server_id_len = server_id_len;
			}

			DBG_INF_FMT("statement data %p", *stmt_data_pp);
		}
	}

	if (server_id) {
		efree(server_id);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::execute */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, execute)(MYSQLND_STMT * const s TSRMLS_DC)
{
	enum_func_status  ret;
	MYSQLND_STMT_DATA * stmt = s->data;
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);

	DBG_ENTER("mysqlnd_qc_ps_stmt::execute");
	if (!(*stmt_data_pp)) {
		DBG_RETURN(orig_mysqlnd_stmt__execute(s TSRMLS_CC));
	}

	DBG_INF("Trying to cache");
	/* ******************** */
	STATS_TIME_SET((*stmt_data_pp)->run_time);
	ret = orig_mysqlnd_stmt__execute(s TSRMLS_CC);
	STATS_TIME_DIFF((*stmt_data_pp)->run_time);
	/* ******************** */
	if (ret == PASS) {
		if (stmt->cursor_exists) {
#if PHP_VERSION_ID < 50400
			QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, s->data->conn);
			MYSQLND_QC_CONNECTION_DATA ** recorded_data_pp = conn_data_pp;
#elif PHP_VERSION_ID >= 50400
			MYSQLND_QC_NET_DATA ** recorded_data_pp =
				(MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(stmt->conn->net, mysqlnd_qc_plugin_id);
#endif
			FREE_SMART_STR_AND_CONTAINER_AND_NULL((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
			MYSQLND_QC_NET_RESTORE_ORIGINAL(stmt->conn, conn_data_pp);
			DBG_INF("there is a cursor, don't perform anything");
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::generate_execute_request */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, generate_execute_request)(MYSQLND_STMT * const s, zend_uchar ** request, size_t *request_len, zend_bool * free_buffer TSRMLS_DC)
{
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);
	enum_func_status ret;

	DBG_ENTER("mysqlnd_qc_ps_stmt::generate_execute_request");

	ret = orig_mysqlnd_stmt__generate_execute_request(s, request, request_len, free_buffer TSRMLS_CC);
	if (ret == PASS && (*stmt_data_pp)) {
		MYSQLND_STMT_DATA * stmt = s->data;
		MYSQLND_CONN_DATA * conn = stmt->conn;
#if PHP_VERSION_ID < 50400
		QC_DECLARE_AND_LOAD_CONN_DATA(recorded_data_pp, conn);
#elif PHP_VERSION_ID >= 50400
		MYSQLND_QC_NET_DATA ** recorded_data_pp =
			(MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(stmt->conn->net, mysqlnd_qc_plugin_id);
#endif
		char * query_hash_key = NULL;
		size_t query_hash_key_len = 0;

		if ((*recorded_data_pp)->recorded_data) {
			DBG_INF("cleaning previous recorded data");
			smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
			mnd_free((*recorded_data_pp)->recorded_data);
			(*recorded_data_pp)->recorded_data = NULL;
		}


		DBG_INF_FMT("query_len="MYSQLND_SZ_T_SPEC" request_len="MYSQLND_SZ_T_SPEC, (*stmt_data_pp)->query_len, *request_len);
		{
			size_t ps_key_len;
			char * ps_key = NULL;
			unsigned char * base64_key = NULL;
			int base64_key_len = 0;

			/* skip the first 4 bytes, which are the stmt_id or we will need exact match of the stmt_id, which will be very rate */
			base64_key = php_base64_encode(*request + 4, *request_len - 4, &base64_key_len);

			ps_key_len = (*stmt_data_pp)->query_len + base64_key_len;
			ps_key = mnd_emalloc(ps_key_len + 1);
			memcpy(ps_key, base64_key,  base64_key_len);
			memcpy(ps_key + base64_key_len, (*stmt_data_pp)->query, (*stmt_data_pp)->query_len);
			ps_key[ps_key_len] = '\0';

			efree(base64_key);

			/* TODO: server_id support */
			query_hash_key = MYSQLND_QC_G(handler)->get_hash_key(conn, ps_key, ps_key_len, &query_hash_key_len, (*stmt_data_pp)->server_id, (*stmt_data_pp)->server_id_len, stmt->persistent TSRMLS_CC);
			mnd_efree(ps_key);
		}

		/* from previous execute */
		if ((*stmt_data_pp)->query_hash_key) {
			pefree((*stmt_data_pp)->query_hash_key, stmt->persistent);
			(*stmt_data_pp)->query_hash_key = NULL;
		}

		if (query_hash_key) {
			smart_str * cached_query = MYSQLND_QC_G(handler)->find_query_in_cache(query_hash_key, query_hash_key_len TSRMLS_CC);

			(*stmt_data_pp)->query_hash_key = query_hash_key;
			(*stmt_data_pp)->query_hash_key_len = query_hash_key_len;

			if (cached_query) {
				DBG_INF("Found it in the cache");
				MYSQLND_QC_NET_SET_REPLAY(conn);
				(*recorded_data_pp)->recorded_data = cached_query;
				(*recorded_data_pp)->recorded_data_current_position = (size_t)0;
				(*stmt_data_pp)->record_mode = FALSE;
			} else {
				(*stmt_data_pp)->record_mode = TRUE;

				DBG_INF("Not in the cache");
				(*recorded_data_pp)->recorded_data = mnd_calloc(1, sizeof(smart_str));
				DBG_INF_FMT("new recorded_data %p", (*recorded_data_pp)->recorded_data);
				MYSQLND_QC_NET_SET_RECORD(conn);
			}
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_ps_use_or_store_result_handler */
static void
mysqlnd_qc_ps_use_or_store_result_handler(MYSQLND_STMT * const s, MYSQLND_RES * const result, uint64_t store_time, uint64_t row_count TSRMLS_DC)
{
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);
	MYSQLND_STMT_DATA * stmt = s->data;
	MYSQLND_CONN_DATA * conn = stmt->conn;
#if PHP_VERSION_ID < 50400
	QC_DECLARE_AND_LOAD_CONN_DATA(conn_data_pp, conn);
	MYSQLND_QC_CONNECTION_DATA ** recorded_data_pp = conn_data_pp;
#elif PHP_VERSION_ID >= 50400
	MYSQLND_QC_NET_DATA ** recorded_data_pp =
		(MYSQLND_QC_NET_DATA **) mysqlnd_plugin_get_plugin_net_data(conn->net, mysqlnd_qc_plugin_id);
#endif

	DBG_ENTER("mysqlnd_qc_ps_use_or_store_result_handler");
	if ((*stmt_data_pp)->record_mode == TRUE) {
		zend_bool no_table = FALSE;
		DBG_INF("not found, let's save it");
		if (!MYSQLND_QC_G(cache_no_table)) {
			unsigned int i;
			for (i = 0; (i < mysqlnd_num_fields(stmt->result)) && !no_table; i++) {
				const MYSQLND_FIELD *field = mysqlnd_fetch_field_direct(result, i);
				no_table = (field->table_length == 0) ? TRUE : FALSE;
			}
		}
		if (TRUE == no_table) {
			DBG_INF("Don't add to cache - no table name for at least one column");
			smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
			mnd_free((*recorded_data_pp)->recorded_data);
		} else if (PASS == MYSQLND_QC_G(handler)->add_query_to_cache_if_not_exists(result,
																	(*stmt_data_pp)->query_hash_key,
																	(*stmt_data_pp)->query_hash_key_len,
																	(*recorded_data_pp)->recorded_data,
																	(*stmt_data_pp)->TTL? (*stmt_data_pp)->TTL:MYSQLND_QC_G(ttl),
																	(*stmt_data_pp)->run_time,
																	store_time,
																	row_count
																	TSRMLS_CC))
		{
			DBG_INF("Added to the cache");
		} else {
			DBG_INF("Query already cached");
			smart_str_free_ex((*recorded_data_pp)->recorded_data, PERSISTENT_STR);
			mnd_free((*recorded_data_pp)->recorded_data);
		}

		MYSQLND_QC_NET_RESTORE_ORIGINAL(conn, conn_data_pp);
	} else {
		MYSQLND_QC_G(handler)->return_to_cache((*stmt_data_pp)->query_hash_key,
												(*stmt_data_pp)->query_hash_key_len,
												(*recorded_data_pp)->recorded_data
												TSRMLS_CC);
		MYSQLND_QC_G(handler)->update_query_run_time_stats((*stmt_data_pp)->query_hash_key,
													(*stmt_data_pp)->query_hash_key_len,
													(*stmt_data_pp)->run_time,
													store_time
													TSRMLS_CC);
	}
	(*recorded_data_pp)->recorded_data = NULL;
	(*stmt_data_pp)->record_mode = FALSE;
	pefree((*stmt_data_pp)->query_hash_key, stmt->persistent);
	(*stmt_data_pp)->query_hash_key = NULL;
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::store_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, store_result)(MYSQLND_STMT * const s TSRMLS_DC)
{
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);
	uint64_t store_time;
	MYSQLND_RES * result;
	DBG_ENTER("mysqlnd_qc_ps_stmt::store_result");

	if (!(*stmt_data_pp)) {
		DBG_RETURN(orig_mysqlnd_stmt__store_result(s TSRMLS_CC));
	}

	(*stmt_data_pp)->use_result = FALSE;

	STATS_TIME_SET(store_time);
		result = orig_mysqlnd_stmt__store_result(s TSRMLS_CC);
	STATS_TIME_DIFF(store_time);

	mysqlnd_qc_ps_use_or_store_result_handler(s, result, store_time, result->stored_data->row_count TSRMLS_CC);

	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::use_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, use_result)(MYSQLND_STMT * s TSRMLS_DC)
{
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);
	MYSQLND_RES * result;
	DBG_ENTER("mysqlnd_qc_ps_stmt::use_result");

	result = orig_mysqlnd_stmt__use_result(s TSRMLS_CC);
	if (result && (*stmt_data_pp)) {
		(*stmt_data_pp)->use_result = TRUE;
	}
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::fetch */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, fetch)(MYSQLND_STMT * const s, zend_bool * const fetched_anything TSRMLS_DC)
{
	enum_func_status ret;
	DBG_ENTER("mysqlnd_qc_ps_stmt::fetch");

	ret = orig_mysqlnd_stmt__fetch(s, fetched_anything TSRMLS_CC);

	if (ret == PASS && *fetched_anything == FALSE) {
		MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);
		MYSQLND_STMT_DATA * stmt = s->data;

		if ((*stmt_data_pp) && (*stmt_data_pp)->use_result == TRUE) {
			uint64_t store_time = 0;
			mysqlnd_qc_ps_use_or_store_result_handler(s, stmt->result, store_time, stmt->result->unbuf->row_count TSRMLS_CC);
			(*stmt_data_pp)->use_result = FALSE;
		}
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_qc_ps_stmt::dtor */
static enum_func_status
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, dtor)(MYSQLND_STMT * const s, zend_bool implicit TSRMLS_DC)
{
	MYSQLND_STMT_DATA * stmt = s->data;
	zend_bool persistent = stmt->persistent;
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);

	DBG_ENTER("mysqlnd_qc_ps_stmt::dtor");
	DBG_INF_FMT("stmt=%p", stmt);

	mysqlnd_qc_ps_free_stmt_plugin_data(stmt_data_pp, persistent TSRMLS_CC);

	DBG_RETURN(orig_mysqlnd_stmt__dtor(s, implicit TSRMLS_CC));
}
/* }}} */

/* {{{ mysqlnd_qc_ps_stmt::free_stmt_content */
static void
MYSQLND_METHOD(mysqlnd_qc_ps_stmt, free_stmt_content)(MYSQLND_STMT * const s  TSRMLS_DC)
{
	MYSQLND_STMT_DATA * stmt = s->data;
	zend_bool persistent = stmt->persistent;
	MYSQLND_QC_STMT_DATA ** stmt_data_pp = (MYSQLND_QC_STMT_DATA **) mysqlnd_plugin_get_plugin_stmt_data(s, mysqlnd_qc_plugin_id);

	DBG_ENTER("mysqlnd_qc_ps_stmt::free_stmt_content");
	DBG_INF_FMT("stmt=%p", stmt);

	mysqlnd_qc_ps_free_stmt_plugin_data(stmt_data_pp, persistent TSRMLS_CC);

	orig_mysqlnd_stmt_free_stmt_content(s TSRMLS_CC);
	DBG_VOID_RETURN;
}
/* }}} */

/* {{{ mysqlnd_qc_ps_register_hooks */
void
mysqlnd_qc_ps_register_hooks()
{
	struct st_mysqlnd_stmt_methods * orig_mysqlnd_stmt_methods = mysqlnd_stmt_get_methods();

	orig_mysqlnd_stmt__prepare = orig_mysqlnd_stmt_methods->prepare;
	orig_mysqlnd_stmt_methods->prepare	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, prepare);

	orig_mysqlnd_stmt__generate_execute_request = orig_mysqlnd_stmt_methods->generate_execute_request;
	orig_mysqlnd_stmt_methods->generate_execute_request	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, generate_execute_request);

	orig_mysqlnd_stmt__execute = orig_mysqlnd_stmt_methods->execute;
	orig_mysqlnd_stmt_methods->execute	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, execute);

	orig_mysqlnd_stmt__store_result = orig_mysqlnd_stmt_methods->store_result;
	orig_mysqlnd_stmt_methods->store_result	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, store_result);

	orig_mysqlnd_stmt__use_result = orig_mysqlnd_stmt_methods->use_result;
	orig_mysqlnd_stmt_methods->use_result	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, use_result);

	orig_mysqlnd_stmt__fetch = orig_mysqlnd_stmt_methods->fetch;
	orig_mysqlnd_stmt_methods->fetch	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, fetch);

	orig_mysqlnd_stmt__dtor = orig_mysqlnd_stmt_methods->dtor;
	orig_mysqlnd_stmt_methods->dtor	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, dtor);

	orig_mysqlnd_stmt_free_stmt_content = orig_mysqlnd_stmt_methods->free_stmt_content;
	orig_mysqlnd_stmt_methods->free_stmt_content	= MYSQLND_METHOD(mysqlnd_qc_ps_stmt, free_stmt_content);

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
