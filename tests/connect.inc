<?php
	/*
	Default values are "localhost", "root",
	database "stest" and empty password.
	Change the MYSQL_TEST environment values
	if you want to use another configuration
	*/

	$driver    = new mysqli_driver;

	$host      = getenv("MYSQL_TEST_HOST")     ? getenv("MYSQL_TEST_HOST") : "localhost";
	$port      = getenv("MYSQL_TEST_PORT")     ? getenv("MYSQL_TEST_PORT") : 3306;
	$user      = getenv("MYSQL_TEST_USER")     ? getenv("MYSQL_TEST_USER") : "root";
	$passwd    = getenv("MYSQL_TEST_PASSWD")   ? getenv("MYSQL_TEST_PASSWD") : "";
	$db        = getenv("MYSQL_TEST_DB")       ? getenv("MYSQL_TEST_DB") : "test";
	$engine    = getenv("MYSQL_TEST_ENGINE")   ? getenv("MYSQL_TEST_ENGINE") : "MyISAM";
	$socket    = getenv("MYSQL_TEST_SOCKET")   ? getenv("MYSQL_TEST_SOCKET") : null;
	$skip_on_connect_failure  = getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") ? getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") : true;
	$connect_flags = getenv("MYSQL_TEST_CONNECT_FLAGS") ? (int)getenv("MYSQL_TEST_CONNECT_FLAGS") : 0;

	$QC_SQL_HINT = strtolower(sprintf("/*%s*/", MYSQLND_QC_ENABLE_SWITCH));

	$QC_SQL_HINT_ENABLE = "qc";
	if (!preg_match(sprintf("@%s=.+@isU", $QC_SQL_HINT_ENABLE), MYSQLND_QC_ENABLE_SWITCH))
		die(sprintf("EEEK - SQL hints from connect.inc need to be updated, SQL hint is '%s'", MYSQLND_QC_ENABLE_SWITCH));

	$QC_SQL_HINT_DISABLE = "qc=off";
	if ($QC_SQL_HINT_DISABLE =! MYSQLND_QC_ENABLE_SWITCH)
		die(sprintf("EEEK - SQL hints from connect.inc need to be updated, SQL hint is '%s'", MYSQLND_QC_DISABLE_SWITCH));;

	$QC_SQL_HINT_TTL = "qc_ttl";
	if (!preg_match(sprintf("@%s=@is", $QC_SQL_HINT_TTL), MYSQLND_QC_TTL_SWITCH))
		die(sprintf("EEEK - SQL hints from connect.inc need to be updated, SQL hint is '%s'", MYSQLND_QC_TTL_SWITCH));

	$QC_SQL_HINT_SERVER_ID = "qc_sid";
	if (!preg_match(sprintf("@%s=@is", $QC_SQL_HINT_SERVER_ID), MYSQLND_QC_SERVER_ID_SWITCH))
		die(sprintf("EEEK - SQL hints from connect.inc need to be updated, SQL hint is '%s'", MYSQLND_QC_SERVER_ID_SWITCH));

	$IS_MYSQLND = stristr(mysqli_get_client_info(), "mysqlnd");

	if (!function_exists('sys_get_temp_dir')) {
		function sys_get_temp_dir() {

			if (!empty($_ENV['TMP']))
				return realpath( $_ENV['TMP'] );
			if (!empty($_ENV['TMPDIR']))
				return realpath( $_ENV['TMPDIR'] );
			if (!empty($_ENV['TEMP']))
				return realpath( $_ENV['TEMP'] );

			$temp_file = tempnam(md5(uniqid(rand(), TRUE)), '');
			if ($temp_file) {
				$temp_dir = realpath(dirname($temp_file));
				unlink($temp_file);
				return $temp_dir;
			}
			return FALSE;
		}
	}

	if (!function_exists('my_mysqli_connect')) {

		/**
		* Whenever possible, please use this wrapper to make testing ot MYSQLI_CLIENT_COMPRESS (and potentially SSL) possible
		*
		* @param enable_env_flags Enable setting of connection flags through 	env(MYSQL_TEST_CONNECT_FLAGS)?
		*/
		function my_mysqli_connect($host, $user, $passwd, $db, $port, $socket, $enable_env_flags = true) {
			global $connect_flags;

			$flags = ($enable_env_flags) ? $connect_flags : false;

			if ($flags !== false) {
				$link = mysqli_init();
				if (!mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags))
					$link = false;
			} else {
				$link = mysqli_connect($host, $user, $passwd, $db, $port, $socket);
			}

			return $link;
		}

		/**
		* Whenever possible, please use this wrapper to make testing ot MYSQLI_CLIENT_COMPRESS (and potentially SSL) possible
		*
		* @param enable_env_flags Enable setting of connection flags through env(MYSQL_TEST_CONNECT_FLAGS)
		*/
		function my_mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags = 0, $enable_env_flags = true) {
			global $connect_flags;

			if ($enable_env_flags)
				$flags & $connect_flags;

			return mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags);
		}

		class my_mysqli extends mysqli {
			public function __construct($host, $user, $passwd, $db, $port, $socket, $enable_env_flags = true) {
				global $connect_flags;

				$flags = ($enable_env_flags) ? $connect_flags : false;

				if ($flags !== false) {
					parent::init();
					$this->real_connect($host, $user, $passwd, $db, $port, $socket, $flags);
				} else {
					parent::__construct($host, $user, $passwd, $db, $port, $socket);
				}
			}
		}
	} else {
		printf("skip Eeeek/BUG/FIXME I - connect.inc included twice! skipif bug?\n");
	}

	if (extension_loaded('mysql')) {
		if (!function_exists('my_mysql_connect')) {
			/* wrapper to simplify test porting */
			function my_mysql_connect($host, $user, $passwd, $db, $port, $socket, $flags = NULL, $persistent = false) {
				global $connect_flags;

				$flags = ($flags === NULL) ? $connect_flags : $flags;

				if ($socket) {
					$host = sprintf("%s:%s", $host, $socket);
				} else if ($port) {
					$host = sprintf("%s:%s", $host, $port);
				}
				if ($persistent) {
					$link = mysql_pconnect($host, $user, $passwd, $flags);
				} else {
					$link = mysql_connect($host, $user, $passwd, true, $flags);
				}

				if (!$link) {
					printf("[000-a] Cannot connect using host '%s', user '%s', password '****', persistent = %d, [%d] %s\n",
						$host, $user, ($persistent) ? 1 : 0,
						mysql_errno(), mysql_error());
					return false;
				}

				if (!mysql_select_db($db, $link)) {
					printf("[000-b] [%d] %s\n", mysql_errno($link), mysql_error($link));
					return false;
				}

				return $link;
			}
		} else {
			printf("skip Eeeek/BUG/FIXME II - connect.inc included twice! skipif bug?\n");
		}
	}

	if (!function_exists("randsql")) {
		/*
			FIXME

			Cache may be shared among processes.
			We need "unique" queries in some tests. Our approach to unique queries
			may give false-positives but it is very unlikely.
		*/
		function randsql($sql, $add_sql_hint = true, $ttl = NULL) {
			global $QC_SQL_HINT, $QC_SQL_HINT_TTL;
			static $calls = 0;

			if (($calls++ + 10) % 10 == 0)
				mt_srand(floor(microtime(true) * 10000));

			if (!is_null($ttl)) {
				$ttl = sprintf("/*%s=%d*/", $QC_SQL_HINT_TTL, $ttl);
			}

			return sprintf("%s%s%s/* unique_id=%d */ ",
				($add_sql_hint) ? $QC_SQL_HINT : "",
				$ttl,
				$sql,
				mt_rand(0, mt_getrandmax()));
		}
	}
?>