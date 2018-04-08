--TEST--
mysqlnd_qc_get_cache_info()
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
require_once("connect.inc");

if (("user" == $env_handler) || ("userp" != $env_handler))
	die("skip Not available with user handler " . $env_handler);

if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
	die(sprintf("SKIP Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
		$host, $user, $db, $port, $socket));
}

if (!($res = mysqli_query($link, $QC_SQL_HINT . "SELECT 1")) ||
	!($row = mysqli_fetch_assoc($res)) ||
	!($res = mysqli_query($link, $QC_SQL_HINT . "SELECT 1")) ||
	!($row = mysqli_fetch_assoc($res))) {
	die(sprintf("SKIP [%d] %s\n", mysqli_errno($link), mysqli_error($link)));
}

$info = mysqlnd_qc_get_cache_info();
if (empty($info['data']))
	printf("skip Handler %s %s does not report statistics\n", $info['handler'], $info['handler_version']);

?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.time_statistics=1
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require_once("handler.inc");

	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT  . "SELECT id, label FROM test WHERE id = 1")) {
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	$expected = array(
		'run_time' 			=> array(false, false),
		'store_time' 		=> array(false, false),
		'min_run_time' 		=> array(($env_handler == "apc"), true),
		'max_run_time' 		=> array(($env_handler == "apc"), true),
		'avg_run_time' 		=> array(($env_handler == "apc"), true),
		'min_store_time' 	=> array(($env_handler == "apc"), true),
		'avg_store_time' 	=> array(($env_handler == "apc"), true),
		'max_store_time' 	=> array(($env_handler == "apc"), true),
	);
	$info = mysqlnd_qc_get_cache_info();

	foreach ($info['data'] as $key => $stats) {
		if (isset($stats['statistics']))
			$stats = $stats['statistics'];
		foreach ($expected as $field => $prop) {
			$is_null = $prop[0];
			$default_zero = $prop[1];
			if (!array_key_exists($field, $stats)) {
				printf("[002] Missing field '%s'\n", $field);
				continue;
			}
			if ($is_null && !is_null($stats[$field])) {
				printf("[003] '%s' is not NULL, got '%s'/%s\n", $field, var_export($stats[$field], true), gettype($stats[$field]));
			} else if (!$is_null) {
				if ($default_zero && (0 !== $stats[$field])) {
					printf("[004] '%s' should be zero, got '%s'/%s\n", $field, var_export($stats[$field], true), gettype($stats[$field]));
				} else if (!$default_zero && (0 === $stats[$field])) {
					printf("[005] '%s' should not be zero, got '%s'/%s\n", $field, var_export($stats[$field], true), gettype($stats[$field]));
				}
			}
		}
	}

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 1")) {
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	if ($row != $rown) {
		printf("[007] Results differ\n");
		var_dump($row);
		var_dump($rown);
	}

	$info2 = mysqlnd_qc_get_cache_info();
	foreach ($info2['data'] as $key => $stats) {
		if (isset($stats['statistics']))
			$stats = $stats['statistics'];
		foreach ($expected as $field => $not_null) {
			if (!array_key_exists($field, $stats)) {
				printf("[008] Missing field '%s'\n", $field);
				continue;
			}
			if (0 === $stats[$field])
				printf("[009] '%s' should be zero, got '%s'\n", $field, var_export($stats[$field], true));
		}
	}

	if ('1' !== ($tmp = ini_set("mysqlnd_qc.time_statistics", 0)))
		printf("[010] Expecting 1 got %s\n", var_export($tmp, true));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 2")) {
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 2"))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 2")) {
		printf("[013] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row != $rown) {
		printf("[014] Results differ\n");
		var_dump($row);
		var_dump($rown);
	}

	$found = false;
	$info = mysqlnd_qc_get_cache_info();
	foreach ($info['data'] as $key => $stats) {
		if (false === stristr($key, "SELECT id, label FROM test WHERE id = 2"))
			continue;
		$found = true;
		if (isset($stats['statistics']))
			$stats = $stats['statistics'];
		foreach ($expected as $field => $not_null) {
			if (!array_key_exists($field, $stats)) {
				printf("[015] Missing field '%s'\n", $field);
				continue;
			}
			/* NOTE - time set to float(0) not NULL */
			if (0 != $stats[$field])
				printf("[016] '%s' is not 0, got %s\n", $field, var_export($stats[$field], true));
		}
	}

	if (!$found)
		printf("[018] Test is borked: second query not found\n");

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!