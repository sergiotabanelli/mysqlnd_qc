--TEST--
mysqlnd_qc.collect_statistics=0
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
mysqlnd_qc.collect_statistics=0
mysqlnd_qc.time_statistics=0
mysqlnd_qc.slam_defense=0
--FILE--
<?php
	require_once("handler.inc");

	function check_stats($offset, $fields, $stats, $update = false) {
		foreach ($fields as $field => $value) {
			if (array_key_exists($field, $stats)) {
				if ($update && $value[0] == NULL) {
					$fields[$field][0] = $stats[$field];
					$value[0] = $stats[$field];
				}
				switch ($value[1]) {
					case 'nocmp':
						break;
					case 'eq':
					default:
						if ($stats[$field] != $value[0])
							printf("[%03d + 02] %s: expecting %s/%s got %s/%s\n", $offset, $field, $value[0], gettype($value[0]), $stats[$field], gettype($stats[$field]));
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

	$fields = array(
		'cache_put' 						=> array(0, "eq"),
		'cache_miss'  						=> array(0, "eq"),
		'cache_hit'							=> array(0, "eq"),
		'query_not_cached'					=> array(0, "eq"),
		'query_could_cache' 				=> array(0, "eq"),
		'query_should_cache' 				=> array(0, "eq"),
		'query_should_not_cache'			=> array(0, "eq"),
		'query_found_in_cache'				=> array(0, "eq"),
		'query_uncached_other'				=> array(0, "eq"),
		'query_uncached_no_table'			=> array(0, "eq"),
		'query_uncached_no_result'			=> array(0, "eq"),
		'query_uncached_use_result'			=> array(0, "eq"),
		'query_aggr_run_time_cache_hit'		=> array(0, "eq"),
		'query_aggr_run_time_cache_put'		=> array(0, "eq"),
		'query_aggr_run_time_total'			=> array(0, "eq"),
		'query_aggr_store_time_cache_hit'	=> array(0, "eq"),
		'query_aggr_store_time_cache_put'	=> array(0, "eq"),
		'query_aggr_store_time_total'		=> array(0, "eq"),
		'receive_bytes_recorded'			=> array(0, "eq"),
		'receive_bytes_replayed'			=> array(0, "eq"),
		'send_bytes_recorded'				=> array(0, "eq"),
		'send_bytes_replayed'				=> array(0, "eq"),
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
	$fields = check_stats(2, $fields, $stats);

	$query = $QC_SQL_HINT  . "SELECT id FROM test WHERE id = 1";
	if (!$res = mysqli_query($link, $query)) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	/* cache put @ store_result*/
	$fields = check_stats(4, $fields, $stats);

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test"))
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	/* cache hit @ store_result - DELETE no SELECT */
	$stats = mysqlnd_qc_get_core_stats();
	check_stats(6, $fields, $stats);

	$query = $QC_SQL_HINT  . "SELECT id FROM test WHERE id = 1";
	if (!$res = mysqli_query($link, $query)) {
		printf("[007] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(8, $fields, $stats);

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	if ($row != $rown) {
		printf("[009] Results differ, dumping data\n");
		var_dump($row);
		var_dump($rown);
	}

	/* cache put */
	$query = $QC_SQL_HINT  . "SELECT id FROM test WHERE id = 2";
	if (!$res = mysqli_query($link, $query)) {
		printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	mysqli_free_result($res);
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(11, $fields, $stats);

	/* cache miss */
	$query = "SELECT id FROM test WHERE id = 2";
	if (!$res = mysqli_query($link, $query)) {
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	$stats = mysqlnd_qc_get_core_stats();
	$fields = check_stats(12, $fields, $stats);

	mysqli_free_result($res);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done%s