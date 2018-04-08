--TEST--
SQL Hint: server id
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
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
 	require_once("handler.inc");
	require("table.inc");


	$unique_id = mt_rand(0, 100000);
	$sql = sprintf("/*%s*/SELECT id, label FROM test WHERE id = 1/*unique_id=%d*/",
		MYSQLND_QC_ENABLE_SWITCH,
		$unique_id);

	if (!($res = $link->query($sql)) || !($default = $res->fetch_assoc())) {
		printf("[001] [%d] %s\n", $link->errno, $link->error);
	}

	if (!$link->query("UPDATE test SET label = 'b' WHERE id = 1")) {
		printf("[002] [%d] %s\n", $link->errno, $link->error);
	}

	if (!($res = $link->query($sql)) || !($default2 = $res->fetch_assoc())) {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	if ($default != $default2) {
		printf("[004] Default server id request not served from cache.\n");
		var_dump($default);
		var_dump($default2);
	}


	$sql = sprintf("/*%s*//*%s%s*/SELECT id, label FROM test WHERE id = 1/*unique_id=%d*/",
		MYSQLND_QC_ENABLE_SWITCH,
		MYSQLND_QC_SERVER_ID_SWITCH,
		"mycluster_name_or_group_criteria",
		$unique_id);

	if (!($res = $link->query($sql)) || !($server_id_set = $res->fetch_assoc())) {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	if ($default == $server_id_set) {
		printf("[006] Server id ignored, cache entries are identical.\n");
		var_dump($default);
		var_dump($server_id_set);
	}

	if (!$link->query("UPDATE test SET label = 'c' WHERE id = 1")) {
		printf("[007] [%d] %s\n", $link->errno, $link->error);
	}

	if (!($res = $link->query($sql)) || !($server_id_set2 = $res->fetch_assoc())) {
		printf("[008] [%d] %s\n", $link->errno, $link->error);
	}

	if ($server_id_set != $server_id_set2) {
		printf("[009] Server id set request not served from cache.\n");
		var_dump($server_id_set);
		var_dump($server_id_set2);
	}

	/* SQL hint twice */
	/* MISS - regardless of server id because SQL string changed and sql comment striping is deactivated */
	$sql = sprintf("/*%s*//*%s%s*//*%s%s*/SELECT id, label FROM test WHERE id = 1/*unique_id=%d*/",
		MYSQLND_QC_ENABLE_SWITCH,
		MYSQLND_QC_SERVER_ID_SWITCH,
		"mycluster_name_or_group_criteria",
		MYSQLND_QC_SERVER_ID_SWITCH,
		"second_shall_win",
		$unique_id);

	if (!($res = $link->query($sql)) || !($server_id_set_again = $res->fetch_assoc())) {
		printf("[010] [%d] %s\n", $link->errno, $link->error);
	}

	if (($default == $server_id_set_again) || ($server_id_set == $server_id_set_again)) {
		printf("[011] Server id ignored, cache entries are identical.\n");
		var_dump($default);
		var_dump($server_id_set);
		var_dump($server_id_set_again);
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!