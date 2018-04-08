--TEST--
bool mysqlnd_qc_set_storage_handler(string handler)
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	require_once('handler.inc');
	require("skipif.inc");

	$info = mysqlnd_qc_get_cache_info();
	$org_handler = $info['handler'] . " " . $info['handler_version'];

	if (NULL !== ($tmp = @mysqlnd_qc_set_storage_handler()))
		printf("[001] Wrong param count not detected, got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_storage_handler("in", "december", "drinking")))
		printf("[002] Wrong param count not detected, got %s\n", var_export($tmp, true));

	$info = mysqlnd_qc_get_cache_info();
	$handler2 = $info['handler'] . " " . $info['handler_version'];
	if ($org_handler != $handler2)
		printf("[003] Original handler %s not restored? %s\n", $org_handler, $handler2);

	$handlers = array("default", "Default", "deFAULT");

	/* from skipif.inc */
	if (extension_loaded('apc') && isset($available_handler['apc'])) {
		$handlers = array_merge($handlers, array("APC", "apc", "ApC"));
	}
	if (isset($available_handler['memcache']))
		$handlers = array_merge($handlers, array("memCACHE", "memcache", "MEMCACHE"));

	foreach ($handlers as $handler) {
		if ($handler == "user" || $handler == "userp")
			continue;

		if (true !== ($tmp = mysqlnd_qc_set_storage_handler($handler)))
			printf("[004] Valid handler '%s' rejected, got %s\n", $handler, var_export($tmp, true));
	}

	if ("user" != $env_handler && "userp" != $env_handler) {
		if (true !== ($tmp = mysqlnd_qc_set_storage_handler($env_handler)))
			printf("[005] Valid handler rejected, got %s\n", var_export($tmp, true));

		$info = mysqlnd_qc_get_cache_info();
		$handler2 = $info['handler'] . " " . $info['handler_version'];
		if ($org_handler != $handler2)
			printf("[006] Original handler %s not restored? %s\n", $org_handler, $handler2);
	}

	if (false !== ($tmp = mysqlnd_qc_set_storage_handler("")))
		printf("[007] Invalid handler not detected, got %s\n", var_export($tmp, true));

	print "done!";
?>
--EXPECTF--
Catchable fatal error: mysqlnd_qc_set_storage_handler(): Unknown handler '' in %s on line %d