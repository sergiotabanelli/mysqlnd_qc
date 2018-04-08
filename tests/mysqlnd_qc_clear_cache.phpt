--TEST--
mysqlnd_qc_clear_cache()
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
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	require_once("handler.inc");
	require_once("table.inc");

	if (NULL !== ($tmp = @mysqlnd_qc_clear_cache("too many args")))
		printf("[001] Expecting NULL got %s\n", var_export($tmp, true));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$implemented = array(
		'apc'		=> true,
		'memcache'	=> false,
		'default'	=> true,
		'user'		=> true,
		'userp'		=> true,
		'sqlite'	=> true,
	);
	$handler = strtolower($env_handler);
	if (!isset($implemented[$handler]))
		printf("[004] Test needs to be updated for handler '%s'\n", $env_handler);

	if ($implemented[$handler] != ($flushed = mysqlnd_qc_clear_cache())) {
		printf("[005] Unexpected behaviour of handler %s\n", $env_handler);
	}

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rowsn = mysqli_fetch_all($res, MYSQLI_ASSOC);
	mysqli_free_result($res);

	if ($flushed) {
		if (!empty($rowsn)) {
			printf("[007] Cache not flushed!\n");
			var_dump($rowsn);
		}
	} else {
		if ($rowsn != $rows) {
			printf("[008] Unflashed = cached results differ\n");
			var_dump($rows);
			var_dump($rowsn);
		}
	}

	print "done!";
?>
--CLEAN--
<php  require_once("clean_table.inc"); ?>
--EXPECTF--
done!