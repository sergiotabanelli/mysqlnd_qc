<?php
/**
* Main configuration file.
*
* Note: this collection of scripts does not aim to be a "modern"
* PHP web application. It is a collection of stuff that proved
* handy during the development of the MySQLnd Query Cache. No more!
*/

/* FIXME - currently it will only work with APC, explicitly set APC as the default handler */
// mysqlnd_qc_change_handler("apc");

error_reporting(E_ALL);
define("QCM_SQL_HINT", sprintf("/*%s*/", MYSQLND_QC_ENABLE_SWITCH));

/* For mysqlnd_qc_monitor_auto_append_mysql.php */
define("QCM_MYSQL_USER", 	"root");
define("QCM_MYSQL_PASS", 	"root");
define("QCM_MYSQL_HOST", 	"127.0.0.1");
define("QCM_MYSQL_DB", 		"test");
define("QCM_MYSQL_SOCKET", 	"/tmp/mysql.sock");
define("QCM_MYSQL_PORT", 	NULL);
define("QCM_MYSQL_PREFIX",	"qc_auto_append_");

/* Page title */
define("QCM_HTML_TITLE", 					"Mysqlnd Query Cache Monitor");

/* Layout stuff used in mysqlnd_qc_monitor_html_header_inc.php file */
define("QCM_HEADLINE_COLOR", 				"#303060");
define("QCM_CSS_DIV_BACKGROUND", 			"#a0a0a3");
define("QCM_CSS_DIV_BORDER", 				"#333355");
define("QCM_CSS_DIV_TEXT_LIGHT", 			QCM_CSS_DIV_BORDER);
define("QCM_TD_LIST_COLOR0", "				#fcfcff");
define("QCM_TD_LIST_COLOR1", 				"#f0f0f3");
?>