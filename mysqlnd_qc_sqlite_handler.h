/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2008 The PHP Group                                |
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

#ifndef PHP_MYSQLND_QC_SQLITE_HANDLER_H
#define PHP_MYSQLND_QC_SQLITE_HANDLER_H

#define MYSQLND_QC_SQLITE_HANDLER_NAME "sqlite"
#define MYSQLND_QC_SQLITE_HANDLER_VERSION 100000
#define MYSQLND_QC_SQLITE_HANDLER_VERSION_STR "1.0.0"

extern zend_class_entry * mysqlnd_qc_handler_sqlite_class_entry;

extern struct st_mysqlnd_qc_methods MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_qc_sqlite);

PHP_FUNCTION(mysqlnd_qc_handler_default_construct);

#endif	/* PHP_MYSQLND_QC_SQLITE_HANDLER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
