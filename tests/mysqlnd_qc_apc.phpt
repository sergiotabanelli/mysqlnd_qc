--TEST--
APC handler
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");

if (!isset($available_handler['apc']))
	die("SKIP APC handler not available");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_phpt"
--FILE--
<?php
	$info = mysqlnd_qc_get_cache_info();

	mysqlnd_qc_set_is_select("mysqlnd_qc_default_query_is_select");
	if (true !== ($tmp = mysqlnd_qc_set_storage_handler("apc")))
		printf("[001] APC handler rejected, got %s\n", var_export($tmp, true));

	require_once("connect.inc");
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[002] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	if (!mysqli_query($link, "DROP TABLE IF EXISTS test") || !mysqli_query($link, "CREATE TABLE test(id INT, col1 BLOB) ENGINE=" . $engine)) {
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$num_rows = 1;
	$blob = mysqli_real_escape_string($link, str_repeat('a', 4096));
	for ($i = 0; $i < 10; $i++) {
		$query = sprintf("INSERT INTO test(id, col1) VALUES (%d, '%s')", $i, $blob);
		if (!mysqli_query($link, $query)) {
			printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
			var_dump($query);
			break;
		}
	}
	$query1 = sprintf("%sSELECT id, col1 FROM test WHERE id < %d", $QC_SQL_HINT, floor($num_rows / 2));
	$query2 = sprintf("%sSELECT id, col1 FROM test WHERE id >= %d", $QC_SQL_HINT, floor($num_rows / 2));


	if (!$res = mysqli_query($link, $query1)) {
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}


	$row1 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	$info1 = mysqlnd_qc_get_cache_info();
	if ($info1['num_entries'] != 1) {
		printf("[005] Number of cached queries should be 1, got: %d!", $info1['num_entries']);
	}

	if (count($info1['data']) != $info1['num_entries']) {
		printf("[006] %d stats for %d cached queries\n", count($info1['data']), $info1['num_entries']);
	}

	foreach ($info1['data'] as $stats) {
		$stats = $stats['statistics'];
		if (NULL !== $stats['min_run_time'])
			printf("[007] min_run_time seems wrong: %s\n", var_export($stats, true));

		if (NULL !== $stats['max_run_time'])
			printf("[008] max_run_time seems wrong: %s\n", var_export($stats, true));

		if (NULL !== $stats['avg_run_time'])
			printf("[009] avg_run_time seems wrong: %s\n", var_export($stats, true));

		if (NULL !== $stats['min_store_time'])
			printf("[010] min_store_time seems wrong: %s\n", var_export($stats, true));

		if (NULL !== $stats['max_store_time'])
			printf("[011] max_store_time seems wrong: %s\n", var_export($stats, true));

		if (NULL !== $stats['avg_store_time'])
			printf("[012] avg_store_time seems wrong: %s\n", var_export($stats, true));

		if (0 != $stats['cache_hits'])
			printf("[013] cache_hits seems wrong: %s\n", var_export($stats, true));

		if (0 == $stats['run_time'])
			printf("[014] uncached_run_time seems wrong: %s\n", var_export($stats, true));

		if (0 == $stats['store_time'])
			printf("[015] uncached_store_time seems wrong: %s\n", var_export($stats, true));
	}

	if (!$res = mysqli_query($link, $query2)) {
		printf("[016] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row2 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	$info1 = mysqlnd_qc_get_cache_info();
	if ($info1['num_entries'] != 2) {
		printf("[017] Number of cached queries should be 2, got: %d!", $info1['num_entries']);
	}

	if (!mysqli_query($link, "DROP TABLE test"))
		printf("[018] [%d] %s\n", mysqli_errno($link), mysqli_error($link));


	/*
	Under some, unclear circumstances the store operation may be super-fast (<1ms) and the stats may still show max_store_time=0 which makes the test fail. Therefore, we do a couple of more execute/fetch=store operations in a loop to reduce the likeliness of false positives.
	*/
	$end = microtime(true) + 0;
	do {

		if (!$res = mysqli_query($link, $query1)) {
			printf("[019] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
		}

		$row1n = mysqli_fetch_assoc($res);
		mysqli_free_result($res);
		if ($row1 != $row1n) {
			printf("[020] Results should not differ\n");
			var_dump($row1);
			var_dump($row1n);
		}

		if (!$res = mysqli_query($link, $query2)) {
			printf("[021] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
		}

		$row2n = mysqli_fetch_assoc($res);
		mysqli_free_result($res);
		if ($row2 != $row2n) {
			printf("[022] Results should not differ\n");
			var_dump($row2);
			var_dump($row2n);
		}

	}  while (microtime(true) < $end);

	$info2 = mysqlnd_qc_get_cache_info();
	if ($info2['num_entries'] != 2) {
		printf("[023] Number of cached queries should be 2, got: %d!", $info2['num_entries']);
	}

	if (count($info2['data']) != $info2['num_entries']) {
		printf("[024] %d stats for %d cached queries\n", count($info2['data']), $info2['num_entries']);
	}

	foreach ($info2['data'] as $k => $stats) {
		$stats = $stats['statistics'];
		if (!isset($stats['min_run_time']) || NULL === $stats['min_run_time'])
			printf("[025] min_run_time seems wrong: %s\n", var_export($stats, true));

		if (!isset($stats['max_run_time']) || NULL === $stats['max_run_time'])
			printf("[026] max_run_time seems wrong: %s\n", var_export($stats, true));

		if (!isset($stats['avg_run_time']) || NULL === $stats['avg_run_time'])
			printf("[027] avg_run_time seems wrong: %s\n", var_export($stats, true));

		if (!isset($stats['min_store_time']) || NULL === $stats['min_store_time'])
			printf("[028] min_store_time seems wrong: %s\n", var_export($stats, true));

		if (!isset($stats['max_store_time']) || NULL === $stats['max_store_time'])
			printf("[029] max_store_time seems wrong: %s\n", var_export($stats, true));

		if (!isset($stats['avg_store_time']) || NULL === $stats['avg_store_time'])
			printf("[030] avg_store_time seems wrong: %s\n", var_export($stats, true));

		if (!isset($stats['cache_hits']) || NULL === $stats['cache_hits'])
			printf("[031] cache_hits seems wrong: %s\n", var_export($stats, true));

		if (isset($info1[$k])) {
			if ($stats['run_time'] != $info1[$k]['run_time'])
				printf("[032] uncached run time got modified, %f != %f\n", $stats['run_time'], $info1[$k]['run_time']);

			if ($stats['store_time'] != $info1[$k]['store_time'])
				printf("[033] uncached store time got modified, %f != %f\n", $stats['store_time'], $info1[$k]['store_time']);
		}
	}

	/* Please, please consider cache entries private in general! */
	$apc_prefix = ini_get('mysqlnd_qc.apc_prefix');
	$prefix_len = strlen($apc_prefix);
	$num_entries = 0;


	$apc_user_data = apc_cache_info("user");
	foreach ($apc_user_data['cache_list'] as $entry) {
		if (substr($entry['info'], 0, $prefix_len) == $apc_prefix) {
			$num_entries++;
			$known = false;
			$details = apc_fetch($entry['info'], $known);
			if (!$known) {
				printf("[034] Cache inspection failed for '%s'.", $entry['info']);
				continue;
			}
			if (!isset($info2['data'][$entry['info']])) {
				printf("[035] Cache inspection failed for '%s'.", $entry['info']);
				continue;
			}

			foreach ($info2['data'][$entry['info']] as $k => $v)
				if (isset($details[$k]) && $details[$k] != $v) {
					printf("[036] %s - %s != %s\n", $k, var_export($v, true), var_export($details[$k], true));
				}

		}
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!