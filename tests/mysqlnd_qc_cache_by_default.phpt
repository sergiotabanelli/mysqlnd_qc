--TEST--
mysqlnd_qc.cache_by_default (default off, runtime changes)
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');

if (getenv("MYSQLND_QC_HANDLER") == "userp") {
	die("SKIP userp handler will cause recursion");
}
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	require_once("handler.inc");
	require('table.inc');


	if (!$res = mysqli_query($link, "SELECT * FROM test ORDER BY id ASC"))
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows[] = $row;
	printf("[002] %d\n", count($rows));
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id > 3"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, "SELECT * FROM test ORDER BY id ASC"))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows2 = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows2[] = $row;
	mysqli_free_result($res);
	printf("[005] %d\n", count($rows2));

	ini_set('mysqlnd_qc.cache_by_default', true);

	if (!$res = mysqli_query($link, "SELECT * FROM test ORDER BY id ASC"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows[] = $row;
	mysqli_free_result($res);
	printf("[007] %d\n", count($rows));

	if (!mysqli_query($link, "DELETE FROM test WHERE id > 2"))
		printf("[008] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, "SELECT * FROM test ORDER BY id ASC"))
		printf("[009] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows2 = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows2[] = $row;
	mysqli_free_result($res);
	printf("[010] %d\n", count($rows2));

	if (!mysqli_query($link, "DELETE FROM test WHERE id > 1"))
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	ini_set('mysqlnd_qc.cache_by_default', false);

	if (!$res = mysqli_query($link, "SELECT * FROM test ORDER BY id ASC"))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows2 = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows2[] = $row;

	mysqli_free_result($res);
	var_dump($rows2);

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<?php require('clean_table.inc'); ?>
--EXPECTF--
[002] 6
[005] 3
[007] 3
[010] 3
array(1) {
  [0]=>
  array(2) {
    ["id"]=>
    string(1) "1"
    ["label"]=>
    string(1) "a"
  }
}
done!