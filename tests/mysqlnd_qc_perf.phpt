--TEST--
Handler performance - skipped by default
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");

if (!isset($available_handler['apc']))
	die("SKIP APC handler not available");

die("SKIP performance testing only - needs to be enabled manually");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.slam_defense=0
apc.use_request_time=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_perf"
mysqlnd_qc.cache_no_table=1
max_run_time=0
--FILE--
<?php
die("done!");
	require_once('handler.inc');
	require_once("connect.inc");

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[002] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	$cache_hits = 500;
	$rows = 10;

	$col_start = 10;
	$col_inc = 10;
	$col_end = $num_cols + ($col_inc * 50);
	$size_start = 100;
	$size_inc = 400;
	$size_end = $size + ($size_inc * 17);

	$avg_speedup = 0;
	$runs = 0;
	$csv = array();
	for ($num_cols = $col_start; $num_cols <= $col_end; $num_cols += $col_inc) {
		for ($size = $size_start; $size <= $size_end; $size += $size_inc) {
			$per_col_size = $size / $num_cols;

			if (!mysqli_query($link, "DROP TABLE IF EXISTS test")) {
				printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
				break 2;
			}

			$cols = "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY";
			for ($i = 0; $i < $num_cols; $i++) {
				$cols .= sprintf(", col%04d BLOB", $i + 1);
			}
			$query = sprintf("CREATE TABLE test(%s) ENGINE=%s", $cols, $engine);
			if (!mysqli_query($link, $query)) {
				printf("[004] [%d] %s - %s\n", mysqli_errno($link), mysqli_error($link), $query);
				break 2;
			}

			$blob = mysqli_real_escape_string($link, str_repeat('a', $per_col_size));
			$cols = "";
			$values = "";
			$bind = "";
			for ($i = 0; $i < $num_cols; $i++) {
				$cols .= sprintf("col%04d, ", $i + 1);
				$values .= "?, ";
				$bind .= '$null, ';
			}
			$stmt = mysqli_stmt_init($link);
			$query = sprintf("INSERT INTO test(%s) VALUES (%s)", substr($cols, 0, -2), substr($values, 0, -2));
			if (!$stmt->prepare($query)) {
				printf("[005] [%d] %s - %s\n", mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt), $query);
				break 2;
			}

			$null = NULL;
			$bind = sprintf('$stmt->bind_param("%s", %s);', str_repeat('b', $num_cols), substr($bind, 0, -2));
			eval($bind);

			for ($num_rows = 0; $num_rows < $rows; $num_rows++) {
				for ($i = 0; $i < $num_cols; $i++) {
					if (!$stmt->send_long_data($i, $blob)) {
						printf("[006] [%d] %s - %s\n", mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt), $query);
						break 3;
					}
				}
				if (!$stmt->execute()) {
					printf("[007] [%d] %s - %s\n", mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt), $query);
					break 2;
				}
			}
			$stmt->close();

			/* breath time for the server */
			usleep(5000);

			$select = sprintf("%sSELECT *, %d, %d FROM test ORDER BY id ASC", $QC_SQL_HINT, $num_cols, $size);
			if (!$res = mysqli_query($link, $select)) {
				printf("[008] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
				break 2;
			}
			$row = mysqli_fetch_assoc($res);
			while ($tmp = mysqli_fetch_assoc($res))
				;
			mysqli_free_result($res);

			if (!mysqli_query($link, "DROP TABLE test")) {
				printf("[009] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
				break 2;
			}

			/* breath time for the server */
			usleep(10000);
			$stats_before = mysqlnd_qc_get_core_stats();
			for ($i = 0; $i < $cache_hits; $i++) {
				if (!$res = mysqli_query($link, $select)) {
					printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
					break 3;
				}
				$rown = mysqli_fetch_assoc($res);
				while ($tmp = mysqli_fetch_assoc($res))
					;
				mysqli_free_result($res);
				if ($row != $rown) {
					printf("[011] Cached data seems borked\n");
					var_dump($row);
					var_dump($rown);
					break 3;
				}
				if ($i == 0) {
					$stats_first = mysqlnd_qc_get_core_stats();
				}
			}
			$stats_after = mysqlnd_qc_get_core_stats();

			$result_set_size	= $stats_first['receive_bytes_replayed'] - $stats_before['receive_bytes_replayed'];

			$run_time_uncached 	= $stats_first['query_aggr_run_time_total'] - $stats_before['query_aggr_run_time_total'];
			$run_time_cached 	= ($stats_after['query_aggr_run_time_total'] - $stats_first['query_aggr_run_time_total']) / $cache_hits;
			$fac_run_time = $run_time_uncached / $run_time_cached;

			$store_time_uncached 	= $stats_first['query_aggr_store_time_total'] - $stats_before['query_aggr_store_time_total'];
			$store_time_cached 	= ($stats_after['query_aggr_store_time_total'] - $stats_first['query_aggr_store_time_total']) / $cache_hits;
			$fac_store_time = $store_time_uncached / $store_time_cached;

			$fac_total = ($run_time_uncached + $store_time_uncached) / ($run_time_cached + $store_time_cached);

			$runs++;
			$avg_speedup += $fac_total;


			printf("%-40s %d\n", "rows:", $rows);
			printf("%-40s %d\n", "columns:", $num_cols);
			printf("%-40s %d\n", "size:", $size);
			printf("%-40s %d bytes\n", "result set size:", $result_set_size);
			printf("%-40s %d ms\n", "run time uncached:", $run_time_uncached);
			printf("%-40s %d ms\n", "run time cached:", $run_time_cached);
			printf("%-40s %4.2fx\n", "run time factor:", $fac_run_time);
			printf("%-40s %d ms\n", "store time uncached:", $store_time_uncached);
			printf("%-40s %d ms\n", "store time cached:", $store_time_cached);
			printf("%-40s %4.2fx\n", "store time factor:", $fac_store_time);
			printf("%-50s %4.2fx\n", "factor:", $fac_total);
			$csv[$num_cols][$size] = array("bytes_receive_reply" => $result_set_size, "factor" => $fac_total);

			printf("\n\n");
		}
	}

	$flag_size_headlines = false;
	foreach ($csv as $num_cols => $size_array) {
		if (!$flag_size_headlines) {
			$flag_size_headlines = true;
			printf("'';");
			foreach ($size_array as $size => $data)
				printf("'%d (%d) bytes';", $size, $data['bytes_receive_reply']);
			printf("\n");
		}
		printf("'%d columns';", $num_cols);
		foreach ($size_array as $size => $data)
			printf("'%s';", number_format($data['factor'], 3, ",", "."));
		printf("\n");
	}
	printf("\n\n");

	if ($avg_speedup > 0) {
		$avg_speedup = $avg_speedup / $runs;
		printf("%-40s %4.2fx", sprintf("average speedup (%s)", $env_handler), $avg_speedup);
	}
	printf("\n\n");
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!