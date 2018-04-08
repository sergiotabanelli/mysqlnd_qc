--TEST--
User handler - parameter
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

	$verbose = true;
	require_once("user_handler_helper.inc");

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers()))
		printf("[001] Expecting NULL got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash")))
		printf("[002] Expecting NULL got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find")))
		printf("[003] Expecting NULL got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find", "add")))
		printf("[004] Expecting NULL got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find", "add")))
		printf("[005] Expecting NULL got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find", "add", "return_to_cache")))
		printf("[006] Expecting NULL got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find", "add", "return_to_cache", "update_stats")))
		printf("[007] Expecting true got %s\n", var_export($tmp, true));

	if (NULL !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find", "add", "return_to_cache", "update_stats", "get_stats", "clear_cache", "dummy")))
		printf("[008] Expecting NULL got %s\n", var_export($tmp, true));

	if (true !== ($tmp = @mysqlnd_qc_set_user_handlers("get_hash", "find", "add", "return_to_cache", "update_stats", "get_stats", "clear_cache")))
		printf("[009] Expecting true got %s\n", var_export($tmp, true));

	$info = mysqlnd_qc_get_cache_info();
	$handler = $info['handler'] . " " . $info['handler_version'];
	var_dump($handler);

	if (NULL !== ($tmp = mysqlnd_qc_set_user_handlers("", "find", "add", "return_to_cache", "update_stats", "get_stats", "unknown")))
		printf("[009] Expecting NULL and error message got %s\n", var_export($tmp, true));

	print "done!";
?>
--EXPECTF--
get_stats(array (
))
string(%d) "user%s"

Catchable fatal error: mysqlnd_qc_set_user_handlers(): Argument 1 is not a valid callback in %s on line %d