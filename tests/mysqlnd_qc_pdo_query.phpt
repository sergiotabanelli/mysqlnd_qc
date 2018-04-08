--TEST--
PDO_MYSQL - PDO::query with emulated PS
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');

if (!extension_loaded('pdo_mysql')) {
	die('skip mysqlnd_qc: required pdo_mysql extension not available');
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

	$dsn = sprintf("mysql:host=%s;dbname=%s", $host, $db);
	if ($socket)
		$dsn .= sprintf(";unix_socket=%s", $socket);
	else
		$dsn .= sprintf(";port=%d", $port);

	$pdo = new PDO($dsn, $user, $passwd);
	/* otherwise we cannot be sure that we don't use native PS and that we use store_result */
	$pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, 1);

	if (!$stmt = $pdo->query($query))
		printf("[001] [%d] %s\n", $pdo->errorCode(), var_export($pdo->errorInfo(), true));

	$rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
	$stmt->closeCursor();

	if (!$pdo->exec("DELETE FROM test WHERE id > 1"))
		printf("[002] [%d] %s\n", $pdo->errorCode(), var_export($pdo->errorInfo(), true));

	if (!$stmt = $pdo->query($query))
		printf("[003] [%d] %s\n", $pdo->errorCode(), var_export($pdo->errorInfo(), true));

	$rowsn = $stmt->fetchAll(PDO::FETCH_ASSOC);
	$stmt->closeCursor();

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

	$dsn = sprintf("mysql:host=%s;dbname=%s", $host, $db);
	if ($socket)
		$dsn .= sprintf(";unix_socket=%s", $socket);
	else
		$dsn .= sprintf(";port=%d", $port);

	if (!$stmt = $pdo->query($query))
		printf("[005] [%d] %s\n", $pdo->errorCode(), var_export($pdo->errorInfo(), true));

	$rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
	$stmt->closeCursor();

	if (!$pdo->exec("DELETE FROM test"))
		printf("[006] [%d] %s\n", $pdo->errorCode(), var_export($pdo->errorInfo(), true));

	if (!$stmt = $pdo->query($query))
		printf("[007] [%d] %s\n", $pdo->errorCode(), var_export($pdo->errorInfo(), true));

	$rowsn = $stmt->fetchAll(PDO::FETCH_ASSOC);
	$stmt->closeCursor();

	if ($rows == $rowsn) {
		printf("[008] Query has been cached\n");
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