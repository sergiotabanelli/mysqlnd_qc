--TEST--
SQL Hint to set TTL
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
max_execution_time=60
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
 	require_once("handler.inc");
	require("table.inc");

	/*
	NOTE

	See mysqlnd_qc_get_core_stats.phpt for memcache and possible false positives.
	*/

	function delete_and_fetch($offset, $link, $select, $delete, $ttl) {

		$start = microtime(true);
		if (!($res = mysqli_query($link, $select))) {
			printf("[%03d + 01] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}

		$row = mysqli_fetch_assoc($res);
		mysqli_free_result($res);
		if (empty($row)) {
			printf("[%03d + 02] Got no data, test will fail\n", $offset);
			return false;
		}

		if (!mysqli_query($link, $delete)) {
			printf("[%03d + 03] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}


		if (!($res = mysqli_query($link, $select))) {
			printf("[%03d + 04] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}

		$rown = mysqli_fetch_assoc($res);
		mysqli_free_result($res);
		if (empty($rown)) {
			printf("[%03d + 05] Got no data, test will fail\n", $offset);
			return false;
		}

		$duration = microtime(true) - $start;
		if ($duration > $ttl) {
			printf("[%03d + 06] Test run took too long, results will be bogus!\n", $offset);
			return false;
		}

		if ($row != $rown) {
			printf("[%03d + 07] Query has not been cached, dumping data\n", $offset);
			var_dump($row);
			var_dump($rown);
		}

		sleep($ttl + 1);
		$duration = microtime(true) - $start;
		if ($duration < $ttl) {
			printf("[%03d + 08] sleep() didn't work, test will fail\n", $offset);
			return false;
		}

		/* cache entry must have expired */
		if (!($res = mysqli_query($link, $select))) {
			printf("[%03d + 09] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}

		$rown = mysqli_fetch_assoc($res);
		mysqli_free_result($res);
		if (!empty($rown)) {
			printf("[%03d + 10] Got data, SQL hint did not overrule TTL from config\n", $offset);
			return false;
		}

		return true;
	}

	$ttl = 2;
	$sql = randsql("SELECT id FROM test WHERE id = 1", true, $ttl);
	if (!delete_and_fetch(1, $link, $sql, "DELETE FROM test WHERE id = 1", $ttl))
		printf("SQL = %s\n", $sql);

	/* second SQL hint must overrule first one */
	$ttl = 3;
	$sql = sprintf("%s/*%s=%d*//*%s=%d*/%s", $QC_SQL_HINT, $QC_SQL_HINT_TTL, $ttl - 1, $QC_SQL_HINT_TTL, $ttl, randsql("SELECT id FROM test WHERE id = 2", "DELETE FROM test WHERE id = 2"));
	if (!delete_and_fetch(2, $link, $sql, "DELETE FROM test WHERE id = 2", $ttl))
		printf("SQL = %s\n", $sql);

	/* ttl = 0 -> use default */
	$ttl = 0;
	$sql = randsql("SELECT id FROM test WHERE id = 3", true, $ttl);
	if (!delete_and_fetch(3, $link, $sql, "DELETE FROM test WHERE id = 3", ini_get("mysqlnd_qc.ttl")))
		printf("SQL = %s\n", $sql);

	/* invalid -> 0 -> use default */
	$sql = sprintf("%s/*%s=bogus*/%s", $QC_SQL_HINT, $QC_SQL_HINT_TTL, randsql("SELECT id FROM test WHERE id = 4"));
	if (!delete_and_fetch(4, $link, $sql, "DELETE FROM test WHERE id = 4", ini_get("mysqlnd_qc.ttl")))
		printf("SQL = %s\n", $sql);

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!