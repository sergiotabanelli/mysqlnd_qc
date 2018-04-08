--TEST--
mysqlnd_qc_get_cache_info()
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
	var_dump(mysqlnd_qc_get_cache_info());
	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT  . "SELECT id, label FROM test WHERE id = 1")) {
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	var_dump($row);

	$info = mysqlnd_qc_get_cache_info();
	$info['something'] = true;

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 1")) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	var_dump($row);

	/* Note: some handlers will report num_entries and stats others will not! */
	$info2 = mysqlnd_qc_get_cache_info();
	var_dump($info2);

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
array(4) {
  ["num_entries"]=>
  int(0)
  ["handler"]=>
  string(%d) "%s"
  ["handler_version"]=>
  string(%d) "%s"
  ["data"]=>
  array(0) {
  }
}
array(2) {
  ["id"]=>
  string(1) "1"
  ["label"]=>
  string(1) "a"
}
array(2) {
  ["id"]=>
  string(1) "1"
  ["label"]=>
  string(1) "a"
}
array(4) {
  ["num_entries"]=>
  int(%d)
  ["handler"]=>
  string(%d) "%s"
  ["handler_version"]=>
  string(%d) "%s"
  ["data"]=>
  %a
}
done!