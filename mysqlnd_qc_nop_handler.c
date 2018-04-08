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
#include "mysqlnd_qc.h"
#include "mysqlnd_qc_nop_handler.h"

#ifdef ZTS
#include "TSRM.h"
#endif


/* {{{ mysqlnd_qc_handler_nop_query_is_select */
static zend_bool
mysqlnd_qc_handler_nop_query_is_select(const char * query, size_t query_len, uint * ttl TSRMLS_DC)
{
	ttl = 0;
	return FALSE;
}
/* }}} */


/* {{{ proto bool mysqlnd_qc_nop_query_is_select()
 */
PHP_FUNCTION(mysqlnd_qc_nop_query_is_select)
{
	int query_hash_key_len;
	char * query_hash_key;

	DBG_ENTER("zif_mysqlnd_qc_nop_query_is_select");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "s", &query_hash_key, &query_hash_key_len) == FAILURE) {
		DBG_VOID_RETURN;
	}
	{
		uint ttl;
		zend_bool ret = mysqlnd_qc_handler_nop_query_is_select(query_hash_key, (size_t) query_hash_key_len, &ttl TSRMLS_CC);
		if (ret == FALSE) {
			RETVAL_FALSE;
		} else if (ttl == 0) {
			RETVAL_TRUE;
		} else {
			RETVAL_LONG(ttl);
		}
	}
	DBG_VOID_RETURN;
}
/* }}} */


struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_nop) = {
	MYSQLND_QC_NOP_HANDLER_NAME,
	MYSQLND_QC_NOP_HANDLER_VERSION_STR,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
MYSQLND_CLASS_METHODS_END;



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
