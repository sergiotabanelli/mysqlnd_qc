--TEST--
mysqlnd_qc.cache_by_default=1 (check ini, no runtime change)
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
mysqlnd_qc.cache_by_default=1
mysqlnd_qc.ttl=2
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
	require_once("handler.inc");
	require('table.inc');

	if ("memcache" == $env_handler || (false == mysqlnd_qc_clear_cache())) {
		$query = randsql("SELECT * FROM test ORDER BY id ASC", false);
	} else {
		$query = "SELECT * FROM test ORDER BY id ASC";
	}
	printf("query = %s\n", $query);

	if (!$res = mysqli_query($link, $query))
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows[] = $row;
	printf("[002] %d\n", count($rows));
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id > 3"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $query))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows2 = array();
	while ($row = mysqli_fetch_assoc($res))
		$rows2[] = $row;
	mysqli_free_result($res);
	printf("[005] %d\n", count($rows2));

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<?php require('clean_table.inc'); ?>
--EXPECTF--
query = %s
[002] 6
[005] 6
done!