--TEST--
ext/mysql - mysql_query
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');

if (!extension_loaded('mysql')){
	die('skip mysqlnd_qc: required mysql extension not available');
}
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
	/* It does no matter if ext/mysql or ext/myslqi creates aux tables */
	require_once("table.inc");

	if ("memcache" == $env_handler || (false == mysqlnd_qc_clear_cache())) {
		$query = randsql("SELECT * FROM test ORDER BY id ASC");
	} else {
		$query = $QC_SQL_HINT . "SELECT * FROM test ORDER BY id ASC";
	}
	printf("query = %s\n", $query);

	$link = @my_mysql_connect($host, $user, $passwd, $db, $port, $socket);
	if (!$res = mysql_query($query, $link))
		printf("[001] [%d] %s\n", mysql_errno($Link), mysql_error($link));

	$rows = array();
	while ($row = mysql_fetch_assoc($res))
		$rows[] = $row;
	mysql_free_result($res);

	if (!$res = mysql_query("DELETE FROM test WHERE id > 1", $link))
		printf("[002] [%d] %s\n", mysql_errno($Link), mysql_error($link));

	if (!$res = mysql_query($query, $link))
		printf("[003] [%d] %s\n", mysql_errno($Link), mysql_error($link));

	$rowsn = array();
	while ($row = mysql_fetch_assoc($res))
		$rowsn[] = $row;
	mysql_free_result($res);

	if ($rows != $rowsn) {
		printf("[004] Query has not been cached\n");
		var_dump($rows);
		var_dump($rowsn);
	}

	/* Don't cache this one... */

	if ("memcache" == $env_handler || (false == mysqlnd_qc_clear_cache())) {
		$query = randsql("SELECT * FROM test ORDER BY id ASC", false);
	} else {
		$query = "SELECT * FROM test ORDER BY id ASC";
	}
	printf("query = %s\n", $query);

	if (!$res = mysql_query($query, $link))
		printf("[005] [%d] %s\n", mysql_errno($Link), mysql_error($link));

	$rows = array();
	while ($row = mysql_fetch_assoc($res))
		$rows[] = $row;
	mysql_free_result($res);

	if (!$res = mysql_query("DELETE FROM test", $link))
		printf("[006] [%d] %s\n", mysql_errno($Link), mysql_error($link));

	if (!$res = mysql_query($query, $link))
		printf("[007] [%d] %s\n", mysql_errno($Link), mysql_error($link));

	$rowsn = array();
	while ($row = mysql_fetch_assoc($res))
		$rowsn[] = $row;
	mysql_free_result($res);

	if ($rows == $rowsn) {
		printf("[008] Query '%s' has been cached\n", $query);
		var_dump($rows);
		var_dump($rowsn);
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
query = %s
query = %s
done!