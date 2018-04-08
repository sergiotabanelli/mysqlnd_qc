--TEST--
User handler - basic usage
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
mysqlnd_qc.ttl=29
--FILE--
<?php
	$verbose = true;
	require_once("user_handler_helper.inc");

	mysqlnd_qc_set_is_select("is_select");
	if (true !== ($tmp = mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "update_stats", "get_stats", "clear_cache")))
		printf("[001] Expecting true got %s\n", var_export($tmp, true));

	$info = mysqlnd_qc_get_cache_info();
	printf("Handler: %s %s\n", $info['handler'], $info['handler_version']);
	printf("Cache entries: %d\n", $info['num_entries']);

	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DROP TABLE test"))
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

	$info = mysqlnd_qc_get_cache_info();
	printf("Handler: %s %s\n", $info['handler'], $info['handler_version']);
	printf("Cache entries: %d\n", $info['num_entries']);

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
get_stats(array (
))
Handler: user%s
Cache entries: 0
is_select('DROP TABLE IF EXISTS test')
	ret = false
is_select('CREATE TABLE test(id INT, label CHAR(1), PRIMARY KEY(id))%s')
	ret = false
is_select('INSERT INTO test(id, label) VALUES %s')
	ret = false
is_select('%s')
	ret = array (
  'ttl' => 29,
  'server_id' => NULL,
)
get_hash('%s', '%s', '%s', '%s', '%s')
	ret = '%s'
find('%s')
	ret = NULL[0]
add('%s', string[%d], %d, %f, %f, 1)
is_select('DROP TABLE test')
	ret = false
is_select('%s')
	ret = array (
  'ttl' => 29,
  'server_id' => NULL,
)
get_hash('%s', '%s', '%s', '%s', '%s')
	ret = '%s'
find('%s')
	ret = string[%d]
return_to_cache('%s')
update_stats(array (
  0 => '%s',
  1 => %d,
  2 => %d,
))
get_stats(array (
))
Handler: user%s
Cache entries: 1
done!