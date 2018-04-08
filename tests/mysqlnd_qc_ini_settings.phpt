--TEST--
php.ini settings
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	require("handler.inc");

	printf("Default values:\n");
	$expected = array(
		"mysqlnd_qc.ttl",
		"mysqlnd_qc.cache_by_default",
		"mysqlnd_qc.cache_no_table",
		"mysqlnd_qc.use_request_time",
		"mysqlnd_qc.time_statistics",
		"mysqlnd_qc.std_data_copy",
		"mysqlnd_qc.collect_statistics",
		"mysqlnd_qc.collect_statistics_log_file",
		"mysqlnd_qc.collect_query_trace",
		"mysqlnd_qc.query_trace_bt_depth",
		"mysqlnd_qc.enable_qc",
		"mysqlnd_qc.slam_defense",
		"mysqlnd_qc.slam_defense_ttl",
		"mysqlnd_qc.ignore_sql_comments",
		);

	if (function_exists("mysqlnd_qc_get_normalized_query_trace_log"))
		$expected[] = "mysqlnd_qc.collect_normalized_query_trace";

	foreach ($expected as $k => $v) {
		printf("'%s' = ", $v);
		var_dump(ini_get($v));
	}

	if (($ttl = ini_set('mysqlnd_qc.ttl', -1)) !== false)
		printf("[001] -1 not rejected\n");

	if (ini_set('mysqlnd_qc.ttl', 10) === false)
		printf("[002] 10 rejected: %d\n", ini_get('mysqlnd_qc.ttl'));

	if (($tmp = ini_get('mysqlnd_qc.ttl')) != 10)
		printf("[003] 10 rejected: %d\n", ini_get('mysqlnd_qc.ttl'));

	ini_set('mysqlnd_qc.cache_by_default', true);
	if (($tmp = ini_get('mysqlnd_qc.cache_by_default')) != true)
		printf("[004] true rejected: %d\n", $tmp);

	ini_set('mysqlnd_qc.cache_by_default', false);
	if (ini_get('mysqlnd_qc.cache_by_default') != false)
		printf("[005] false rejected\n");

	ini_set('mysqlnd_qc.time_statistics', false);
	if (ini_get('mysqlnd_qc.time_statistics') != false)
		printf("[006] false rejected\n");

	ini_set('mysqlnd_qc.collect_statistics', true);
	if (ini_get('mysqlnd_qc.collect_statistics') != true)
		printf("[007] true rejected\n");

	if (isset($available_handler['apc'])) {

		$expected[] = "mysqlnd_qc.apc_prefix";

		$default = ini_get("mysqlnd_qc.apc_prefix");
		if ($default !== 'qc_')
			printf("[008] New apc_prefix default of '%s'?\n", $default);

		if (false !== ini_set("mysqlnd_qc.apc_prefix", ""))
			printf("[009] Empty apc_prefix accepted\n");

		if ($default !== ($tmp = ini_set("mysqlnd_qc.apc_prefix", "new")))
			printf("[010] Value 'new' rejected: %s\n", var_export($tmp, true));

		if ("new" !== ($tmp = ini_set("mysqlnd_qc.apc_prefix", $default)))
			printf("[011] Original value '%s' rejected: %s\n", $default, var_export($tmp, true));
	}

	if (isset($available_handler['memcache'])) {

		$expected[] = "mysqlnd_qc.memc_server";
		$expected[] = "mysqlnd_qc.memc_port";

		$default = ini_get("mysqlnd_qc.memc_server");
		if (false !== ini_set("mysqlnd_qc.memc_server", ""))
			printf("[012] Empty memc_server accepted\n");

		if ($default !== ($tmp = ini_set("mysqlnd_qc.memc_server", "new")))
			printf("[013] Value 'new' rejected: %s\n", var_export($tmp, true));

		if ("new" !== ($tmp = ini_set("mysqlnd_qc.memc_server", $default)))
			printf("[014] Original value '%s' rejected: %s\n", $default, var_export($tmp, true));

		$default = ini_get("mysqlnd_qc.memc_port");
		if (false !== ini_set("mysqlnd_qc.memc_port", -1))
			printf("[015] Negative 	value for memc_port not rejected\n");

		if ($default !== ($tmp = ini_set("mysqlnd_qc.memc_port", 101)))
			printf("[016] Value 101 rejected: %s\n", var_export($tmp, true));

		if (101 != ($tmp = ini_set("mysqlnd_qc.memc_port", $default)))
			printf("[017] Original value '%s' rejected: %s\n", $default, var_export($tmp, true));
	}

	if (isset($available_handler['sqlite'])) {

		$expected[] = "mysqlnd_qc.sqlite_data_file";

		$default = ini_get("mysqlnd_qc.sqlite_data_file");
		if (false !== ini_set("mysqlnd_qc.sqlite_data_file", ""))
			printf("[018] Empty mysqlnd_qc.sqlite_data_file accepted\n");

		if ($default !== ($tmp = ini_set("mysqlnd_qc.sqlite_data_file", "new")))
			printf("[019] Value 'new' rejected: %s\n", var_export($tmp, true));

		if ("new" !== ($tmp = ini_set("mysqlnd_qc.sqlite_data_file", $default)))
			printf("[020] Original value '%s' rejected: %s\n", $default, var_export($tmp, true));

	}

	if (false !== ini_set('mysqlnd_qc.std_data_copy', 1)) {
		printf("[021] std_data_copy is not PHP_INI_SYSTEM\n");
		if (($tmp = ini_get('mysqlnd_qc.std_data_copy')) != 1) {
			printf("[022] true rejected: %d\n", $tmp);
		}
	}

	ini_set('mysqlnd_qc.std_data_copy', false);
	if (ini_get('mysqlnd_qc.std_data_copy') != false)
		printf("[023] false rejected\n");

	ob_start();
	phpinfo(INFO_MODULES);
	$output = ob_get_contents();
	ob_end_clean();

	foreach ($expected as $k => $v) {
		$needle = sprintf("%s =>", $v);
		if (FALSE !== stristr($output, $needle)) {
			$output = str_replace($needle, "", $output, $count);
		} else {
			printf("[041] '%s' not found in phpinfo() output\n", $v);
		}
	}

	ini_set('mysqlnd_qc.slam_defense', true);
	if (ini_get('mysqlnd_qc.slam_defense') == true)
		printf("[026] Is mysqlnd_qc.slam_defense no longer INI_SYSTEM?\n");

	ini_set('mysqlnd_qc.collect_query_trace', true);
	if (ini_get('mysqlnd_qc.collect_query_trace') == true)
		printf("[027] Is mysqlnd_qc.collect_query_trace no longer INI_SYSTEM?\n");

	ini_set('mysqlnd_qc.query_trace_bt_depth', 12);
	if (ini_get('mysqlnd_qc.query_trace_bt_depth') == 12)
		printf("[028] Is mysqlnd_qc.query_trace_bt_depth no longer INI_SYSTEM?\n");

	ini_set('mysqlnd_qc.enable_qc', false);
	if (ini_get('mysqlnd_qc.enable_qc') == false)
		printf("[029] Is mysqlnd_qc.enable_qc no longer INI_SYSTEM?\n");

	ini_set('mysqlnd_qc.collect_query_trace', true);
	if (ini_get('mysqlnd_qc.collect_query_trace') == true)
		printf("[030] Is mysqlnd_qc.collect_query_trace no longer INI_SYSTEM?\n");

	if (in_array("mysqlnd_qc.collect_normalized_query_trace", $expected)) {
		ini_set('mysqlnd_qc.collect_normalized_query_trace', true);
		if (ini_get('mysqlnd_qc.collect_normalized_query_trace') == true)
			printf("[031] Is mysqlnd_qc.collect_normalized_query_trace no longer INI_SYSTEM?\n");
	}

	ini_set('mysqlnd_qc.slam_defense_ttl', 10);
	if (ini_get('mysqlnd_qc.slam_defense_ttl') == 10)
		printf("[032] Is mysqlnd_qc.slam_defense_ttl no longer INI_SYSTEM?\n");

	print "done!";
?>
--EXPECTF--
Default values:
'mysqlnd_qc.ttl' = string(2) "30"
'mysqlnd_qc.cache_by_default' = string(1) "0"
'mysqlnd_qc.cache_no_table' = string(1) "0"
'mysqlnd_qc.use_request_time' = string(1) "0"
'mysqlnd_qc.time_statistics' = string(1) "1"
'mysqlnd_qc.std_data_copy' = string(1) "0"
'mysqlnd_qc.collect_statistics' = string(1) "0"
'mysqlnd_qc.collect_statistics_log_file' = string(21) "/tmp/mysqlnd_qc.stats"
'mysqlnd_qc.collect_query_trace' = string(1) "0"
'mysqlnd_qc.query_trace_bt_depth' = string(1) "3"
'mysqlnd_qc.enable_qc' = string(1) "1"
'mysqlnd_qc.slam_defense' = string(1) "0"
'mysqlnd_qc.slam_defense_ttl' = string(2) "30"
'mysqlnd_qc.ignore_sql_comments' = string(1) "1"
'mysqlnd_qc.collect_normalized_query_trace' = string(1) "0"
done!