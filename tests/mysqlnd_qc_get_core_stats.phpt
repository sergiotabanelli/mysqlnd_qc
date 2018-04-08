--TEST--
array mysqlnd_qc_get_core_stats() - statistics collected by the query cache core (see inline memcache notes if it fails with memcache)
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
mysqlnd_qc.collect_statistics=1
mysqlnd_qc.time_statistics=1
mysqlnd_qc.slam_defense=0
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
	require_once("handler.inc");

	/*
		NOTE

		The test may give wrong results with memcache because
		memcache and userp test storage handler remain their
		data between two test runs. We try to work around
		this by randomizing the query strings. However, there
		is no warranty that randomize works. It should
		not happen but you may get a false positive.
		If the test fails with memcache, please restart
		memcache to flush all data or wait 60s (>> mysqlnd_qc.ttl)
		before running the test again.
	*/

	function check_stats($offset, $fields, $stats, $update = false) {
		foreach ($fields as $field => $value) {
			if (array_key_exists($field, $stats)) {
				if ($update && $value[0] == NULL) {
					$fields[$field][0] = $stats[$field];
					$value[0] = $stats[$field];
				}
				switch ($value[1]) {
					case 'lt':
						if ($stats[$field] <= $value[0])
							printf("[%03d + 01] %s: expecting less than %d got %d\n", $offset, $field, $value[0], $stats[$field]);
						if ($update)
							$fields[$field][0] = $stats[$field];
						break;
					case 'lteq':
						if ($stats[$field] < $value[0])
							printf("[%03d + 01] %s: expecting less than or equal of %d got %d\n", $offset, $field, $value[0], $stats[$field]);
						if ($update)
							$fields[$field][0] = $stats[$field];
						break;
					case 'nocmp':
						break;
					case 'eq':
					default:
						if ($stats[$field] != $value[0])
							printf("[%03d + 02] %s: expecting %d got %d\n", $offset, $field, $value[0], $stats[$field]);
						break;
				}

				unset($stats[$field]);
			} else {
				printf("[%03d + 03] Field '%s' not found\n", $offset, $field);
			}
		}
		foreach ($stats as $field => $value) {
			printf("[%03d + 04] Unexpected field '%s' = %d\n", $offset, $field, $value[0]);
		}
		return $fields;
	}

	if (NULL !== ($tmp = @mysqlnd_qc_get_core_stats(1, 2)))
		printf("[001] Expecting NULL got %s/%s\n", gettype($tmp), $tmp);

	$fields = array(
		'cache_put' 						=> array(0, NULL),
		'cache_miss'  						=> array(0, NULL),
		'cache_hit'							=> array(0, NULL),
		'query_not_cached'					=> array(0, NULL),
		'query_could_cache' 				=> array(0, NULL),
		'query_should_cache' 				=> array(0, NULL),
		'query_should_not_cache'			=> array(0, NULL),
		'query_found_in_cache'				=> array(0, NULL),
		'query_uncached_other'				=> array(0, NULL),
		'query_uncached_no_table'			=> array(0, NULL),
		'query_uncached_no_result'			=> array(0, NULL),
		'query_uncached_use_result'			=> array(0, NULL),
		'query_aggr_run_time_cache_hit'		=> array(0, "eq"),
		'query_aggr_run_time_cache_put'		=> array(0, "eq"),
		'query_aggr_run_time_total'			=> array(0, "eq"),
		'query_aggr_store_time_cache_hit'	=> array(0, "eq"),
		'query_aggr_store_time_cache_put'	=> array(0, "eq"),
		'query_aggr_store_time_total'		=> array(0, "eq"),
		'receive_bytes_recorded'			=> array(0, NULL),
		'receive_bytes_replayed'			=> array(0, NULL),
		'send_bytes_recorded'				=> array(0, NULL),
		'send_bytes_replayed'				=> array(0, NULL),
		/* TODO - test */
		'slam_stale_refresh'				=> array(0, "eq"),
		'slam_stale_hit'					=> array(0, "eq"),
		'request_counter'					=> array(1, "eq"),
		'process_hash'						=> array(NULL, "nocmp"),
	);
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(2, $fields, $stats);

	/* No queries have been run so far */

	require("table.inc");
	$stats = mysqlnd_qc_get_core_stats();
	foreach ($fields as $field => $value)
		$fields[$field] = array(NULL, $value[1]);
	/* let the function update the field values, we don't know what table.inc has done */
	$fields = check_stats(2, $fields, $stats, true);
	$fields['query_aggr_run_time_cache_hit'][1] = "lteq";
	$fields['query_aggr_run_time_cache_put'][1] = "lteq";
	$fields['query_aggr_run_time_total'][1] = "lteq";
	$fields['query_aggr_store_time_cache_hit'][1] = "lteq";
	$fields['query_aggr_store_time_cache_put'][1] = "lteq";
	$fields['query_aggr_store_time_total'][1] = "lteq";
	$fields['receive_bytes_recorded'][1] = "lteq";
	$fields['receive_bytes_replayed'][1] = "lteq";
	$fields['send_bytes_recorded'][1] = "lteq";
	$fields['send_bytes_replayed'][1] = "lteq";
	$fields['process_hash'][1] = "eq";

	$query = randsql("SELECT id FROM test WHERE id = 1");
	/* without randomize: $query = $QC_SQL_HINT  . "SELECT id FROM test WHERE id = 1"; */
	if (!$res = mysqli_query($link, $query)) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	/* cache put @ store_result*/

	$fields['cache_put'][0]++;
	$fields['cache_miss'][0]++;
	$fields['query_could_cache'][0]++;
	$fields['query_should_cache'][0]++;
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(4, $fields, $stats, true);

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test"))
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	/* cache hit @ store_result - DELETE no SELECT */
	$fields['query_not_cached'][0]++;
	$fields['query_should_not_cache'][0]++;
	$stats = mysqlnd_qc_get_core_stats();
	check_stats(6, $fields, $stats);

	/* without randomize:  $query = $QC_SQL_HINT  . "SELECT id FROM test WHERE id = 1"; */
	if (!$res = mysqli_query($link, $query)) {
		printf("[007] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$fields['cache_hit'][0]++;
	$fields['query_could_cache'][0]++;
	$fields['query_should_cache'][0]++;
	$fields['query_found_in_cache'][0]++;
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(8, $fields, $stats, true);

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	if ($row != $rown) {
		printf("[009] Results differ, dumping data\n");
		var_dump($row);
		var_dump($rown);
	}

	/* cache put */

	$query = randsql("SELECT id FROM test WHERE id = 2");
	/* without randomize:  $query = $QC_SQL_HINT  . "SELECT id FROM test WHERE id = 2"; */
	if (!$res = mysqli_query($link, $query)) {
		printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	$fields['cache_put'][0]++;
	$fields['cache_miss'][0]++;
	$fields['query_could_cache'][0]++;
	$fields['query_should_cache'][0]++;
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(11, $fields, $stats, true);

	mysqli_free_result($res);

	/* cache miss */
	$query = randsql("SELECT id FROM test WHERE id = 2", false);
	/* without randomize: $query = "SELECT id FROM test WHERE id = 2"; */
	if (!$res = mysqli_query($link, $query)) {
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$fields['query_should_not_cache'][0]++;
	$fields['query_not_cached'][0]++;
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(12, $fields, $stats, true);

	mysqli_free_result($res);

	/* time to check hits */

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done%s