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
#include "php_mysqlnd_qc.h"
#include "mysqlnd_qc_zval_util.h"

#ifdef ZTS
#include "TSRM.h"
#endif


/* {{{ mysqlnd_qc_add_to_array_string */
void
mysqlnd_qc_add_to_array_string(zval * array, const char * key, size_t key_len, const char * const value, size_t value_len TSRMLS_DC)
{
#if MYSQLND_UNICODE
	UChar *ustr, *tstr;
	int ulen, tlen;

	zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, key, key_len + 1 TSRMLS_CC);
	zend_string_to_unicode(UG(utf8_conv), &tstr, &tlen, value, value_len + 1 TSRMLS_CC);
	add_u_assoc_unicode_ex(array, IS_UNICODE, ZSTR(ustr), ulen, tstr, 1);
	efree(ustr);
	efree(tstr);
#else
	add_assoc_stringl_ex(array, key, key_len + 1, (char *) value, value_len, 1);
#endif
}
/* }}} */


/* {{{ mysqlnd_qc_add_to_array_null */
void
mysqlnd_qc_add_to_array_null(zval * array, const char * key, size_t key_len TSRMLS_DC)
{
#if MYSQLND_UNICODE
	UChar *ustr;
	int ulen;

	zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, key, key_len + 1 TSRMLS_CC);
	add_u_assoc_null_ex(array, IS_UNICODE, ZSTR(ustr), ulen);
	efree(ustr);
#else
	add_assoc_null_ex(array, key, key_len + 1);
#endif
}
/* }}} */


/* {{{ mysqlnd_qc_add_to_array_long */
void
mysqlnd_qc_add_to_array_long(zval * array, const char * key, size_t key_len, long value TSRMLS_DC)
{
#if MYSQLND_UNICODE
	UChar *ustr;
	int ulen;

	zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, key, key_len + 1 TSRMLS_CC);
	add_u_assoc_long_ex(array, IS_UNICODE, ZSTR(ustr), ulen, value);
	efree(ustr);
#else
	add_assoc_long_ex(array, key, key_len + 1, value);
#endif
}
/* }}} */


/* {{{ mysqlnd_qc_add_to_array_zval */
void
mysqlnd_qc_add_to_array_zval(zval * array, const char * key, size_t key_len, zval * value TSRMLS_DC)
{
#if MYSQLND_UNICODE
	UChar *ustr;
	int ulen;

	zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, key, key_len + 1 TSRMLS_CC);
	add_u_assoc_zval_ex(array, IS_UNICODE, ZSTR(ustr), ulen, value);
	efree(ustr);
#else
	add_assoc_zval_ex(array, key, key_len + 1, value);
#endif
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
