--TEST--
Temporary for isolated issues
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.ignore_sql_comments=0
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

	require("handler.inc");
	require("table.inc");

	$obj = new mysqlnd_qc_handler_default();
	mysqlnd_qc_set_is_select("mysqlnd_qc_default_query_is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[001] Usage of mysqlnd_qc_handler_default failed\n");
	}

	$sql = randsql("SELECT id FROM test WHERE id = 1");

	printf("SQL = %s\n", $sql);
	$res = $link->query($sql);
	var_dump($res->fetch_assoc());
	$res->free();

	printf("SQL = %s\n", $sql);
	$res = $link->query($sql);
	var_dump($res->fetch_assoc());
	$res->free();

	print "done!";

?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
SQL = %s
array(1) {
  ["id"]=>
  string(1) "1"
}
SQL = %s
array(1) {
  ["id"]=>
  string(1) "1"
}
done!