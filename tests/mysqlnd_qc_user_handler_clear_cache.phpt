--TEST--
User handler - mysqlnd_qc_clear_cache() callback
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
--FILE--
<?php
	$verbose = false;
	require_once("user_handler_helper.inc");

	mysqlnd_qc_set_is_select("is_select");
	if (true !== ($tmp = mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "update_stats", "get_stats", "clear_cache_void")))
		printf("[001] Expecting true got %s\n", var_export($tmp, true));

	$info = mysqlnd_qc_get_cache_info();
	printf("Handler: %s %s\n", $info['handler'], $info['handler_version']);

	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
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

	/* clear_cache_void() will not flush the cache */
	var_dump(mysqlnd_qc_clear_cache());

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rowsn = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if ($rows != $rowsn) {
		printf("[007] Results should not differ, cache flushed?\n");
		var_dump($rows);
		var_dump($rowsn);
	}

	mysqlnd_qc_set_is_select("is_select");
	if (true !== ($tmp = mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "update_stats", "get_stats", "clear_cache")))
		printf("[008] Expecting true got %s\n", var_export($tmp, true));

	/* clear_cache() will flush the cache */
	var_dump(mysqlnd_qc_clear_cache());

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[009] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rowsn = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if ($rows == $rowsn) {
		printf("[010] Results should not differ, cache flushed?\n");
		var_dump($rows);
		var_dump($rowsn);
	}

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
Handler: user %s
bool(false)
bool(true)
done!