--TEST--
mysqlnd_qc_get_cache_info(), default handler
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--INI--
mysqlnd_qc.ignore_sql_comments=0
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_phpt"
--FILE--
<?php
	require_once("handler.inc");
	if (!mysqlnd_qc_set_storage_handler("default")) {
		printf("[001] Failed to switch to default handler\n");
	}

	require("table.inc");
	$sql = sprintf("%s/*%s=%d*/%s", $QC_SQL_HINT, $QC_SQL_HINT_TTL, 3, "SELECT id, label FROM test WHERE id = 1");
	if (!$res = mysqli_query($link, $sql)) {
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);

	if (!$link->query("DELETE FROM test WHERE id = 1"))
		printf("[003] [%d] %s\n", $link->errno, $link->error);

	if (!$res = $link->query($sql))
		printf("[004] [%d] %s\n", $link->errno, $link->error);

	$row2 = $res->fetch_assoc();
	if ($row != $row2) {
		printf("[005] Cache miss, dumping data\n");
		var_dump($row);
		var_dump($row2);
	}

	/* should be ignore because QC hint */
	$sql .= sprintf("/*%s=10*/", $QC_SQL_HINT_TTL);

	if (!$res = $link->query($sql))
		printf("[006] [%d] %s\n", $link->errno, $link->error);

	$row2 = $res->fetch_assoc();
	if ($row != $row2) {
		printf("[007] Cache miss, dumping data\n");
		var_dump($row);
		var_dump($row2);
	}

	/* should be ignored althouh  NO QC hint - cache hit */
	$sql .= "/*!AND 1=1*/";
	if (!$res = $link->query($sql))
		printf("[008] [%d] %s\n", $link->errno, $link->error);

	$row2 = $res->fetch_assoc();
	if ($row != $row2) {
		printf("[009] Cache miss, dumping data\n");
		var_dump($row);
		var_dump($row2);
	}

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
[009] Cache miss, dumping data
array(2) {
  ["id"]=>
  string(1) "1"
  ["label"]=>
  string(1) "a"
}
NULL
done!