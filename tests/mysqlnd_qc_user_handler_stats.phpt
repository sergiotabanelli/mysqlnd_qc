--TEST--
User handler - statistics (not linked to myslqnd_qc_get_cache_info())
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.cache_no_table=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.time_statistics=1
--FILE--
<?php
	require_once("user_handler_helper.inc");

	mysqlnd_qc_set_is_select("is_select");
	if (true !== ($tmp = mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "update_stats", "get_stats", "clear_cache")))
		printf("[001] Expecting true got %s\n", var_export($tmp, true));

	$info = mysqlnd_qc_get_cache_info();
	printf("Handler: %s %s\n", $info['handler'], $info['handler_version']);

	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DROP TABLE test"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rowsn = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if ($rows != $rowsn) {
		printf("[005] Results should not differ\n");
		var_dump($rows);
		var_dump($rowsn);
	}

	/* TODO: user handler stats do not appear in mysqlnd_qc_get_cache_info() */
	$verbose = true;
	$info = mysqlnd_qc_get_cache_info();
	if (1 !== $info['num_entries'])
		printf("[006] num_entries is %d\n", $info['num_entries']);
	if (empty($info['data']))
		printf("[007] data is empty\n");

	$stats = get_stats();
	foreach ($stats as $key => $details) {
		$run_time = "";
		$store_time = "";

		foreach ($details['run_time'] as $time) {
			$run_time .= sprintf("%2.5f, ", $time);
			if (0 == $time)
				printf("[008] Suspicious run_time of 0\n");
		}

		foreach ($details['store_time'] as $time) {
			$store_time .= sprintf("%2.5f, ", $time);
			if (0 == $time)
				printf("[009] Suspicious store_time of 0\n");
		}

		printf("%s - run_time = %s, store_time = %s\n", $key, substr($run_time, 0, -2), substr($run_time, 0, -2));
	}

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
Handler: user %s
get_stats(array (
))
get_stats(array (
))
%s - run_time = %f, store_time = %f
done!