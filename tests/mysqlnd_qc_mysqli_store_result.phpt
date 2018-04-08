--TEST--
mysqli_store_result - cached
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
--FILE--
<?php
 	require_once("handler.inc");
	require("table.inc");
	if (!mysqli_real_query($link, $QC_SQL_HINT  . "SELECT id, label FROM test WHERE id = 1")) {
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$res = mysqli_store_result($link);
	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_real_query($link, $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 1")) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$res = mysqli_store_result($link);
	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row != $rown || empty($row)) {
		printf("[004] Results seem wrong\n");
		var_dump($row);
		var_dump($rown);
	}

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!