--TEST--
mysqlnd_qc_get_query_trace_log()
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
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
mysqlnd_qc.collect_query_trace="1"
mysqlnd_qc.collect_normalized_query_trace="0"
mysqlnd_qc.query_trace_bt_depth=1
mysqlnd_qc.cache_no_table=1
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
	/*
	NOTE

	See mysqlnd_qc_get_core_stats.phpt for memcache and possible false positives.
	*/
	require_once("handler.inc");

	function dump_trace($trace) {
		$expected = array(
			'query' 					=> 'string',
			'origin'					=> 'string',
			'run_time'					=> 'int',
			'store_time'				=> 'int',
			'eligible_for_caching' 		=> 'bool',
			'no_table'					=> 'bool',
			'was_added'					=> 'bool',
			'was_already_in_cache'		=> 'bool',
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

	if (NULL !== ($tmp = @mysqlnd_qc_get_query_trace_log("foO")))
		printf("[001] Expecting NULL got %s/%s\n", gettype($tmp), var_export($tmp, true));

	if (!is_array($tmp = mysqlnd_qc_get_query_trace_log()) || (0 !== count($tmp)))
		printf("[002] Expecting empty array got %s/%s\n", gettype($tmp), var_export($tmp, true));

	require_once("connect.inc");
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[003] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	$sql = randsql("SELECT 1", false);
	if (!$res = mysqli_query($link, $sql))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$select_one = mysqlnd_qc_get_query_trace_log();
	dump_trace($select_one[0]);

	require("table.inc");
	$table_inc = mysqlnd_qc_get_query_trace_log();
	if (!is_array($table_inc) || (count($select_one) > count($table_inc)))
		printf("[005] Expecting non-empty array got %s/%s\n", gettype($table_inc), var_export($table_inc, true));

	if ($table_inc[0] != $select_one[0]) {
		printf("[006] Looks like existing query trace entries got modified and/or removed, dumping data\n");
		var_dump($select_one[0]);
		var_dump($table_inc);
	}

	$stats_before = mysqlnd_qc_get_core_stats();
	$sql = randsql("SELECT id FROM test ORDER BY id ASC");
	if (!$res = mysqli_query($link, $sql))
		printf("[007] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);
	$query_trace = mysqlnd_qc_get_query_trace_log();
	$stats_after = mysqlnd_qc_get_core_stats();
	$run_time = $stats_after['query_aggr_run_time_total'] - $stats_before['query_aggr_run_time_total'];
	$store_time = $stats_after['query_aggr_store_time_total'] - $stats_before['query_aggr_store_time_total'];
	if ($stats_before['cache_put'] != ($stats_after['cache_put'] - 1))
		printf("[008] No cache put, test will fail\n");

	if (count($query_trace) != (count($table_inc) + 1)) {
		printf("[009] Expecting one more trace log entry, dumping data\n");
		var_dump($query_trace);
		var_dump($table_inc);
	}

	$last_query_trace = $query_trace[count($table_inc)];
	if ($last_query_trace["run_time"] != $run_time) {
		printf("[010] Run time should be %d got %d\n", $run_time, $last_query_trace['run_time']);
	}
	if ($last_query_trace["store_time"] != $store_time) {
		printf("[011] Store time should be %d got %d\n", $store_time, $last_query_trace['store_time']);
	}
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	$stats_before = mysqlnd_qc_get_core_stats();
	if (!$res = mysqli_query($link, $sql))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);
	$query_trace = mysqlnd_qc_get_query_trace_log();
	$stats_after = mysqlnd_qc_get_core_stats();
	$run_time = $stats_after['query_aggr_run_time_total'] - $stats_before['query_aggr_run_time_total'];
	$store_time = $stats_after['query_aggr_store_time_total'] - $stats_before['query_aggr_store_time_total'];
	if ($stats_before['cache_hit'] != ($stats_after['cache_hit'] - 1))
		printf("[013] No cache hit, test will fail\n");

	if (count($query_trace) != (count($last_query_trace) + 1)) {
		printf("[014] Expecting one more trace log entry, dumping data\n");
		var_dump($query_trace);
		var_dump($last_query_trace);
	}
	$last_query_trace = $query_trace[count($last_query_trace)];
	if ($last_query_trace["run_time"] != $run_time) {
		printf("[015] Run time should be %d got %d\n", $run_time, $last_query_trace['run_time']);
	}
	if ($last_query_trace["store_time"] != $store_time) {
		printf("[016] Store time should be %d got %d\n", $store_time, $last_query_trace['store_time']);
	}
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	$stats_before = mysqlnd_qc_get_core_stats();
	$sql = randsql("SELECT 1");
	if (!$res = mysqli_query($link, $sql))
		printf("[017] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	$query_trace = mysqlnd_qc_get_query_trace_log();
	$last_query_trace = $query_trace[count($last_query_trace)];
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	if (!$res = mysqli_query($link, $sql))
		printf("[018] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	$query_trace = mysqlnd_qc_get_query_trace_log();
	$stats_after = mysqlnd_qc_get_core_stats();
	if ($stats_before['cache_hit'] != ($stats_after['cache_hit'] - 1))
		printf("[019] No cache hit, test will fail\n");

	$last_query_trace = $query_trace[count($last_query_trace)];
	dump_trace($last_query_trace);
	$last_query_trace = $query_trace;

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
query                         : s(SELECT 1/*%s*/ )
origin                        : s(#0 %s(%d): mysqli_query(Object(mysqli), 'SELECT 1/*%s')
)
run_time                      : i(%d)
store_time                    : i(%d)
eligible_for_caching          : b(false)
no_table                      : b(false)
was_added                     : b(false)
was_already_in_cache          : b(false)

query                         : s(/*%s*/SELECT id FROM test ORDER BY id ASC/*%s*/ )
origin                        : s(#0 %s(%d): mysqli_query(Object(mysqli), '/*%s*/SELECT...')
)
run_time                      : i(%d)
store_time                    : i(%d)
eligible_for_caching          : b(true)
no_table                      : b(false)
was_added                     : b(true)
was_already_in_cache          : b(false)

query                         : s(/*%s*/SELECT id FROM test ORDER BY id ASC/*%s*/ )
origin                        : s(#0 %s(%d): mysqli_query(Object(mysqli), '/*%s*/SELECT...')
)
run_time                      : i(%d)
store_time                    : i(%d)
eligible_for_caching          : b(true)
no_table                      : b(false)
was_added                     : b(false)
was_already_in_cache          : b(true)

query                         : s(/*%s*/SELECT 1/*%s*/ )
origin                        : s(#0 %s(%d): mysqli_query(Object(mysqli), '/*%s*/SELECT...')
)
run_time                      : i(%d)
store_time                    : i(%d)
eligible_for_caching          : b(true)
no_table                      : b(false)
was_added                     : b(true)
was_already_in_cache          : b(false)

query                         : s(/*%s*/SELECT 1/*%s*/ )
origin                        : s(#0 %s(%d): mysqli_query(Object(mysqli), '/*%s*/SELECT...')
)
run_time                      : i(%d)
store_time                    : i(%d)
eligible_for_caching          : b(true)
no_table                      : b(false)
was_added                     : b(false)
was_already_in_cache          : b(true)

done!