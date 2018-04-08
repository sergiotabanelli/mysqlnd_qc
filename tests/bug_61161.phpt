--TEST--
Bug #61161 MySQL SUBSTRING function disables caching
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
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
 	require_once("connect.inc");

	/* Not a bug: cache_no_table must be set */

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
	}

	if (!$link->query("DROP TABLE IF EXISTS test") ||
		!$link->query("CREATE TABLE test(id INT, label CHAR(50), PRIMARY KEY(id)) ENGINE=" . $engine) ||
		!$link->query("INSERT INTO test(id, label) VALUES (1, 'This is a'), (2, 'somewhat silly test')")) {
		printf("[002] [%d] %s\n", $link->errno, $link->error);
	}

	/* uncached */
	$sql = randsql("SELECT label FROM test ORDER BY id ASC");
	if (!($res = $link->query($sql))) {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	while ($row = $res->fetch_assoc()) {
		printf("%s-", trim($row['label']));
	}
	printf("\n");

	$sql = randsql("SELECT id, SUBSTRING(label, 9, 14) AS _short_label FROM test LIMIT 2");
	if (!$res = $link->query($sql)) {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

	while ($row = $res->fetch_assoc()) {
		printf("%s-", trim($row['_short_label']));
	}
	printf("\n");

	/* cached */
	$sql = randsql("SELECT id, SUBSTRING(label, 9, 14) AS _short_label FROM test LIMIT 2", true, 10);
	if (!$res = $link->query($sql)) {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	while ($row = $res->fetch_assoc()) {
		printf("%s-", trim($row['_short_label']));
	}
	printf("\n");

	if (!$link->query("DELETE FROM test")) {
		printf("[006] [%d] %s\n", $link->errno, $link->error);
	}

	printf("Served from cache:\n");

	/* cached */
	if (!$res = $link->query($sql)) {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	while ($row = $res->fetch_assoc()) {
		printf("%s-", trim($row['_short_label']));
	}
	printf("\n");

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
This is a-somewhat silly test-
a-silly test-
a-silly test-
Served from cache:
a-silly test-
done!