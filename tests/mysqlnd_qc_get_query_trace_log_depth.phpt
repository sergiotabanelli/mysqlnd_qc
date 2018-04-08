--TEST--
mysqlnd_qc_get_[normalized]_query_trace_log(), mysqlnd_qc.query_trace_bt_depth=2
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
require_once("handler.inc");
if ("userp" == $env_handler)
	die("skip Not supported with handler");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.time_statistics=1;
mysqlnd_qc.collect_statistics=1
mysqlnd_qc.collect_query_trace=1
mysqlnd_qc.collect_normalized_query_trace=1
mysqlnd_qc.query_trace_bt_depth=4
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require_once("handler.inc");
	require_once("connect.inc");

	function analyze_bt($trace) {
		$max_depth = 0;
		foreach ($trace as $k => $bt)  {
			printf("%-20s: %s\n", "query", $bt['query']);
			$origin = preg_split("@#\d@isu", $bt['origin'], 0,  PREG_SPLIT_NO_EMPTY);
			if (count($origin) > $max_depth)
				$max_depth = count($origin);
			printf("%-20s: %s\n", "depth", count($origin));
			foreach ($origin as $k => $v) {
				if (preg_match("@([^(]+)\((\d+)\)\:(.*)@isu", $v, $matches)) {
					printf("%-20s: %d - %d - %s\n", "origin", $k, $matches[2], trim($matches[3]));
				} else {
					// main and stuff
					printf("%-20s: %s\n", "origin", $k, trim($v));
				}
			}
			printf("\n");
		}
		return $max_depth;
	}

	function level1($link, $query) {
		return level2($link, $query);
	}
	function level2($link, $query) {
		return level3($link, $query);
	}
	function level3($link, $query) {
		return level4($link, $query);
	}
	function level4($link, $query) {
		return mysqli_query($link, $query);
	}

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	$query = $QC_SQL_HINT . "SELECT 1";
	if (!$res = level1($link, $query)) {
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$query = $QC_SQL_HINT . "SELECT 1, 2";
	if (!$res = level4($link, $query)) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$query = $QC_SQL_HINT . "SELECT 1, 2, 3";
	if (!$res = level3($link, $query)) {
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$query = $QC_SQL_HINT . "SELECT 1, 2, 3, 4";
	if (!$res = level2($link, $query)) {
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	if (($max_depth = analyze_bt(mysqlnd_qc_get_query_trace_log())) > 4)
		printf("[005] Max depth must not be larger than 4 - found %d!\n", $max_depth);

	/* PHP_INI_SYSTEM - shall be rejected! */
	var_dump(ini_set("mysqlnd_qc.query_trace_bt_depth", 1));

	if (($max_depth = analyze_bt(mysqlnd_qc_get_query_trace_log())) > 4)
		printf("[006] Max depth must not be larger than 4 - found %d!\n", $max_depth);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
query               : /*%s*/SELECT 1
depth               : 4
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2 - %d - level3(Object(mysqli), '/*%s*/SELECT...')
origin              : 3 - %d - level2(Object(mysqli), '/*%s*/SELECT...')

query               : /*%s*/SELECT 1, 2
depth               : 3
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2

query               : /*%s*/SELECT 1, 2, 3
depth               : 4
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2 - %d - level3(Object(mysqli), '/*%s*/SELECT...')
origin              : 3

query               : /*%s*/SELECT 1, 2, 3, 4
depth               : 4
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2 - %d - level3(Object(mysqli), '/*%s*/SELECT...')
origin              : 3 - 59 - level2(Object(mysqli), '/*%s*/SELECT...')

bool(false)
query               : /*%s*/SELECT 1
depth               : 4
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2 - %d - level3(Object(mysqli), '/*%s*/SELECT...')
origin              : 3 - %d - level2(Object(mysqli), '/*%s*/SELECT...')

query               : /*%s*/SELECT 1, 2
depth               : 3
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2

query               : /*%s*/SELECT 1, 2, 3
depth               : 4
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2 - %d - level3(Object(mysqli), '/*%s*/SELECT...')
origin              : 3

query               : /*%s*/SELECT 1, 2, 3, 4
depth               : 4
origin              : 0 - %d - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - level4(Object(mysqli), '/*%s*/SELECT...')
origin              : 2 - %d - level3(Object(mysqli), '/*%s*/SELECT...')
origin              : 3 - 59 - level2(Object(mysqli), '/*%s*/SELECT...')

done!