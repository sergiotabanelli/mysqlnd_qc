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

#ifndef PHP_MYSQLND_QC_ZVAL_UTIL_H
#define PHP_MYSQLND_QC_ZVAL_UTIL_H

void mysqlnd_qc_add_to_array_string(zval * array, const char * key, size_t key_len, const char * const value, size_t value_len TSRMLS_DC);
void mysqlnd_qc_add_to_array_null(zval * array, const char * key, size_t key_len TSRMLS_DC);
void mysqlnd_qc_add_to_array_long(zval * array, const char * key, size_t key_len, long value TSRMLS_DC);
void mysqlnd_qc_add_to_array_double(zval * array, const char * key, size_t key_len, double value TSRMLS_DC);
void mysqlnd_qc_add_to_array_zval(zval * array, const char * key, size_t key_len, zval * value TSRMLS_DC);



#define QC_LONG(val, a)						\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_LONG((a), (val));					\
}

#define QC_DOUBLE(val, a)					\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_DOUBLE((a), (val));				\
}

#define QC_STRING(vl, a)					\
{											\
	char *__vl = (vl);						\
	QC_STRINGL(__vl, strlen(__vl), (a));	\
}

#define QC_STRINGL(vl, ln, a)				\
{											\
	MAKE_STD_ZVAL((a));						\
	ZVAL_STRINGL((a), (char *)(vl), (ln), 1);	\
}

#define QC_BOOL(val, a)				\
{											\
	MAKE_STD_ZVAL(a);						\
	ZVAL_BOOL((a), (val));	\
}


#endif	/* PHP_MYSQLND_QC_ZVAL_UTIL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
