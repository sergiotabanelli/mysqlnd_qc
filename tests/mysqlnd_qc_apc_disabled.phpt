--TEST--
Trying to force use of APC handler although APC is disabled
--SKIPIF--
<?php
require_once('skipif.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");

if (!isset($available_handler['apc']))
	die("SKIP APC handler not available");
?>
--INI--
apc.enabled=0
apc.enable_cli=1
apc.use_request_time=1
apc.slam_defense=1
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	$info = mysqlnd_qc_get_cache_info();

	function catchable_fatal($errno, $errstr, $errfile, $errline) {
		switch ($errno) {
			case E_WARNING:
				$type = "Warning";
				break;
			case E_RECOVERABLE_ERROR:
				$type = "Catchable fatal error";
				break;
			default:
				$type = "Other";
				break;
		}
		printf("%s: [%d] %s in %s on line %d\n\n", $type, $errno, $errstr, $errfile, $errline);
	}
	set_error_handler("catchable_fatal", E_RECOVERABLE_ERROR);

	if (false !== ($tmp = mysqlnd_qc_set_storage_handler("apc")))
			printf("[001] APC handler not rejected, got %s\n", var_export($tmp, true));

	print "done!";
?>
--EXPECTF--
Warning: mysqlnd_qc_set_storage_handler(): APC is disabled (apc.enabled=0), cannot use APC for storage. You must enable APC to use it in %s on line %d

Warning: mysqlnd_qc_set_storage_handler(): APC slam defense (apc.slam_defense=1) will clash with fast cache updates. You must disable it to be able to use the APC handler in %s on line %d

Warning: mysqlnd_qc_set_storage_handler(): APC and MYSQLND_QC are configured to use different timers. TTL invalidation may fail. You should use the same settings for mysqlnd_qc.use_request_time and apc.use_request_time in %s on line %d

Warning: mysqlnd_qc_set_storage_handler(): Error during changing handler. Init of 'apc' failed in %s on line %d
done!