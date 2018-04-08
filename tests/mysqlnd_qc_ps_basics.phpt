--TEST--
Cached prepared statements
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
mysqlnd_qc.ttl=4
mysqlnd_qc.use_request_time=0
max_execution_time=60
mysqlnd_qc.ignore_sql_comments=0
mysqlnd.debug=d:t:O,/tmp/mysqlnd.trace
--FILE--
<?php
 	require_once("handler.inc");
	require("table.inc");

	function run_test_query($offset, $link, $sql, $id_in) {

		if (!($stmt = mysqli_stmt_init($link)) || !mysqli_stmt_prepare($stmt, $sql)) {
			printf("[%03d + 1] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}

		if (!mysqli_stmt_bind_param($stmt, "i", $id_in) || !mysqli_stmt_execute($stmt)) {
			printf("[%03d + 2] [%d] %s\n", $offset, mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt));
			return false;
		}

		$id = $label = NULL;
		if (!mysqli_stmt_bind_result($stmt, $id, $label)) {
			printf("[%04d + 3] [%d] %s\n", $offset, mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt));
			return false;
		}

		$ret = array();
		while (mysqli_stmt_fetch($stmt))
			$ret[] = array($id, $label);

		mysqli_stmt_close($stmt);
		return $ret;
	}

	$sql =  randsql("SELECT id, label FROM test WHERE id = ?");
	$data = run_test_query(10, $link, $sql, 1);
	if (!is_array($data) || empty($data))
		printf("[011] No or empty result set\n");

	// check if cache has been filled
	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	// this shall give a cache hit
	$cached_data = run_test_query(20, $link, $sql, 1);
	if (!is_array($cached_data) || empty($cached_data))
		printf("[013] No or empty result set\n");

	if ($data != $cached_data) {
		printf("[014] Cached result set seems wrong. Dumping uncached and cached results\n");
		var_dump($data);
		var_dump($cached_data);
	}

	// TTL SQL hint
	$sql = sprintf("%s/*%s=%d*/%s",
		$QC_SQL_HINT,
		$QC_SQL_HINT_TTL,
		2,
		randsql("SELECT id, label FROM test WHERE id > ?", false));

	$data = run_test_query(30, $link, $sql, 1);
	if (!is_array($data) || empty($data))
		printf("[015] No or empty result set\n");

	// check if cache has been filled
	if (!mysqli_query($link, "DELETE FROM test WHERE id > 1"))
		printf("[016] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	// this shall give a cache hit
	$cached_data = run_test_query(40, $link, $sql, 1);
	if (!is_array($cached_data) || empty($cached_data))
		printf("[017] No or empty result set\n");

	if ($data != $cached_data) {
		printf("[018] Cached result set seems wrong. Dumping uncached and cached results\n");
		var_dump($data);
		var_dump($cached_data);
	}

	sleep(3);

	$cached_data = run_test_query(50, $link, $sql, 1);
	if (!empty($cached_data))
		printf("[018] Data has not expired\n");


	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!
