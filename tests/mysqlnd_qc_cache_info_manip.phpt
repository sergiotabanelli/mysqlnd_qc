--TEST--
Manipulate return value of mysqlnd_qc_get_cache_info()
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");

if (!isset($available_handler['apc']))
	die("SKIP APC handler not available");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_info_manip"
--FILE--
<?php
	require_once('handler.inc');
	$available_handler = array("default","apc");

	foreach ($available_handler as $handler) {
		if ($handler == 'user' || $handler == 'userp')
			continue;

		$info1 = mysqlnd_qc_get_cache_info();
		
		if (true !== ($tmp = mysqlnd_qc_set_storage_handler($handler)))
			printf("[001] '%s' handler rejected, got %s\n", $handler, var_export($tmp, true));

		require("table.inc");
		if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1")) {
			printf("[002] '%s' [%d] %s\n", $handler, mysqli_errno($link), mysqli_error($link));
		}

		$row = mysqli_fetch_assoc($res);
		mysqli_free_result($res);

		$info2 = mysqlnd_qc_get_cache_info();
		/* check for refcounting errors and similar */
		$info2['something'] = true;

		$info3 = mysqlnd_qc_get_cache_info();
	}

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<?php require('clean_table.inc'); ?>
--EXPECTF--
done!