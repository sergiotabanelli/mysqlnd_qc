--TEST--
mysqlnd_qc_get_query_trace_log()
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
mysqlnd_qc.collect_query_trace="0"
mysqlnd_qc.collect_normalized_query_trace="1"
mysqlnd_qc.query_trace_bt_depth=1
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require_once("handler.inc");

	function dump_trace($trace) {
		$expected = array(
			'query' 					=> 'string',
			'min_run_time'				=> 'int',
			'avg_run_time'				=> 'int',
			'max_run_time'				=> 'int',
			'min_store_time'			=> 'int',
			'avg_store_time'			=> 'int',
			'max_store_time'			=> 'int',
			'eligible_for_caching' 		=> 'bool',
			'occurences'				=> 'int',
		);

		foreach ($expected as $field => $type) {
			switch ($type) {
				case 'int':
					printf("%-30s: i(%d)\n", $field, $trace[$field]);
					break;
				case 'bool':
					printf("%-30s: b(%s)\n", $field, $trace[$field] ? 'true' : 'false');
					break;
				case 'string':
				default:
					printf("%-30s: s(%s)\n", $field, $trace[$field]);
					break;
			}
			unset($trace[$field]);
		}
		printf("\n");

		if (count($trace)) {
			printf("Unexpected columns! You need to update the test.");
			var_dump($trace);
		}
	}

	if (NULL !== ($tmp = @mysqlnd_qc_get_normalized_query_trace_log("foO")))
		printf("[001] Expecting NULL got %s/%s\n", gettype($tmp), var_export($tmp, true));

	if (!is_array($tmp = mysqlnd_qc_get_normalized_query_trace_log()) || (0 !== count($tmp)))
		printf("[002] Expecting empty array got %s/%s\n", gettype($tmp), var_export($tmp, true));

	require_once("connect.inc");
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[003] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	if (!$res = mysqli_query($link, "SELECT 1, '?', '\''"))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$select_one = mysqlnd_qc_get_normalized_query_trace_log();
	dump_trace($select_one[0]);

	require("table.inc");
	$table_inc = mysqlnd_qc_get_normalized_query_trace_log();
	if (!is_array($table_inc) || (count($select_one) > count($table_inc)))
		printf("[005] Expecting non-empty array got %s/%s\n", gettype($table_inc), var_export($table_inc, true));

	if ($table_inc[0] != $select_one[0]) {
		printf("[006] Looks like existing query trace entries got modified and/or removed, dumping data\n");
		var_dump($select_one[0]);
		var_dump($table_inc);
	}

	$stats_before = mysqlnd_qc_get_core_stats();
	$query = $QC_SQL_HINT . "SELECT id FROM test ORDER BY id ASC";
	if (!$res = mysqli_query($link, $query))
		printf("[007] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);
	$query_trace = mysqlnd_qc_get_normalized_query_trace_log();
	$stats_after = mysqlnd_qc_get_core_stats();
	$run_time = $last_run_time = $stats_after['query_aggr_run_time_total'] - $stats_before['query_aggr_run_time_total'];
	$store_time = $last_store_time = $stats_after['query_aggr_store_time_total'] - $stats_before['query_aggr_store_time_total'];

	if (count($query_trace) != (count($table_inc) + 1)) {
		printf("[008] Expecting one more trace log entry, dumping data\n");
		var_dump($query_trace);
		var_dump($table_inc);
	}

	$last_query_trace = $query_trace[count($table_inc)];
	if (($last_query_trace["min_run_time"] != $run_time) || ($last_query_trace["avg_run_time"] != $run_time) || ($last_query_trace["max_run_time"] != $run_time)) {
		printf("[009] All run times should be %d got min_run_time = %d, avg_run_time = %d, max_run_time = %d\n",
			$run_time,
			$last_query_trace['min_run_time'],
			$last_query_trace['avg_run_time'],
			$last_query_trace['max_run_time']);
	}

	if (($last_query_trace["min_store_time"] != $store_time) || ($last_query_trace["avg_store_time"] != $store_time) || ($last_query_trace["max_store_time"] != $store_time)){
		printf("[010] All store times should be %d got min_store_time = %d, avg_store_time = $d, max_store_time = %d\n",
			$store_time,
			$last_query_trace['min_store_time'],
			$last_query_trace['avg_store_time'],
			$last_query_trace['max_store_time']);
	}
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	$stats_before = mysqlnd_qc_get_core_stats();
	if (!$res = mysqli_query($link, $query))
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);
	$query_trace = mysqlnd_qc_get_normalized_query_trace_log();
	$stats_after = mysqlnd_qc_get_core_stats();
	$run_time = $stats_after['query_aggr_run_time_total'] - $stats_before['query_aggr_run_time_total'];

	$store_time = $stats_after['query_aggr_store_time_total'] - $stats_before['query_aggr_store_time_total'];
	if ($stats_before['cache_hit'] != ($stats_after['cache_hit'] - 1))
		printf("[012] No cache hit, test will fail\n");

	if (count($query_trace) != (count($last_query_trace))) {
		printf("[013] Expecting no more trace log entries, dumping data\n");
		var_dump($query_trace);
		var_dump($last_query_trace);
	}

	$last_query_trace = $query_trace[count($last_query_trace) - 1];

	$min_run_time = min($run_time, $last_run_time);
	$avg_run_time = (int)(($run_time + $last_run_time) / 2);
	$max_run_time = max($run_time, $last_run_time);
	if ($last_query_trace["min_run_time"] != $min_run_time) {
		printf("[014] min_run_time should be %d got %d\n", $min_run_time, $last_query_trace['min_run_time']);
	}
	if (($last_query_trace["avg_run_time"] < ($avg_run_time - 1)) ||
		($last_query_trace["avg_run_time"] > ($avg_run_time + 1))) {
		printf("[015] avg_run_time should be %d (+/- 1) got %d\n", $avg_run_time, $last_query_trace['avg_run_time']);
	}
	if ($last_query_trace["max_run_time"] != $max_run_time) {
		printf("[016] max_run_time should be %d got %d\n", $max_run_time, $last_query_trace['max_run_time']);
	}

	$min_store_time = min($store_time, $last_store_time);
	$avg_store_time = (int)(($store_time + $last_store_time) / 2);
	$max_store_time = max($store_time, $last_store_time);
	if ($last_query_trace["min_store_time"] != $min_store_time) {
		printf("[017] min_store_time should be %d got %d\n", $min_store_time, $last_query_trace['min_store_time']);
	}
	if (($last_query_trace["avg_store_time"] < ($avg_store_time - 1)) ||
		($last_query_trace["avg_store_time"] > ($avg_store_time + 1))) {
		printf("[018] avg_store_time should be %d  (+/- 1) got %d\n", $avg_store_time, $last_query_trace['avg_store_time']);
	}
	if ($last_query_trace["max_store_time"] != $max_store_time) {
		printf("[019] max_store_time should be %d got %d\n", $max_store_time, $last_query_trace['max_store_time']);
	}

	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	$stats_before = mysqlnd_qc_get_core_stats();
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT 1"))
		printf("[020] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	$query_trace = mysqlnd_qc_get_normalized_query_trace_log();
	$last_query_trace = $query_trace[count($last_query_trace)];
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	if ((1 != ini_set('mysqlnd_qc.cache_no_table', "0")) || (0 != ini_get('mysqlnd_qc.cache_no_table'))) {
		printf("[021] mysqlnd_qc_cache_no_table not set to 0, got %s", ini_get('mysqlnd_qc.cache_no_table'));
	}

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT 1"))
		printf("[022] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	$query_trace = mysqlnd_qc_get_normalized_query_trace_log();
	$stats_after = mysqlnd_qc_get_core_stats();
	if ($stats_before['cache_hit'] != ($stats_after['cache_hit'] - 1))
		printf("[023] No cache hit, test will fail\n");

	$last_query_trace = $query_trace[count($last_query_trace) - 1];
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	$stats_before = mysqlnd_qc_get_core_stats();
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT 2, 'a'"))
		printf("[024] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);
	$stats_after = mysqlnd_qc_get_core_stats();
	if ($stats_before['cache_put'] != ($stats_after['cache_put']))
		printf("[025] Cache put, test will fail\n");

	$query_trace = mysqlnd_qc_get_normalized_query_trace_log();
	$last_query_trace = $query_trace[count($last_query_trace)];
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	$stats_before = mysqlnd_qc_get_core_stats();
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT 2, 'a'"))
		printf("[026] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);
	$stats_after = mysqlnd_qc_get_core_stats();

	if ($stats_before['cache_hit'] != ($stats_after['cache_hit']))
		printf("[027] Cache hit, test will fail\n");

	$eligible = ((($stats_after['query_not_cached'] - $stats_before['query_not_cached']) == 0) &&
		($stats_after['query_should_cache'] == $stats_before['query_should_cache'] + 1));

	$query_trace = mysqlnd_qc_get_normalized_query_trace_log();
	$last_query_trace = $query_trace[count($last_query_trace) - 1];
	dump_trace($last_query_trace);
	if ($last_query_trace['eligible_for_caching'] != $eligible) {
		printf("[028] Trace or stats are wrong/have changed. Check manually.\n");
	}
	$last_query_trace = $query_trace;

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
query                         : s(SELECT ?, ?, ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(false)
occurences                    : i(0)

query                         : s(SELECT id FROM test ORDER BY id ASC)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

query                         : s(SELECT id FROM test ORDER BY id ASC)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

query                         : s(SELECT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

query                         : s(SELECT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

query                         : s(SELECT ?, ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

query                         : s(SELECT ?, ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

done!