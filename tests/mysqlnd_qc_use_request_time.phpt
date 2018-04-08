--TEST--
Fast but inprecise SAPI time
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--INI--
mysqlnd_qc.use_request_time=1
mysqlnd_qc.ttl=1
--FILE--
<?php
	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT  . "SELECT id, label FROM test WHERE id = 1"))
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	$info = mysqlnd_qc_get_cache_info();
	if ($info['num_entries'] != 1)
		printf("[002] Num cache entries: %d ?!", $info['num_entries']);

	sleep(2);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 1"))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row2 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row != $row2) {
		printf("[005] Timeout should not have been recognized because of SAPI time.\n");
		var_dump($row);
		var_dump($row2);
	}

	if ($info['num_entries'] != 1)
		printf("[006] Num cache entries: %d ?!", $info['num_entries']);

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!