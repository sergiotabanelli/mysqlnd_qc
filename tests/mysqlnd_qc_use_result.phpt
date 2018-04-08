--TEST--
mysqli_use_result - uncached!
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
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
 	require_once("handler.inc");
	require("table.inc");
	$sql = randsql("SELECT id, label FROM test WHERE id = 1");
	if (!mysqli_real_query($link, $sql)) {
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$res = mysqli_use_result($link);
	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	var_dump($row);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_real_query($link, $sql)) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$res = mysqli_use_result($link);
	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	var_dump($row);

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
array(2) {
  ["id"]=>
  string(1) "1"
  ["label"]=>
  string(1) "a"
}
NULL
done!