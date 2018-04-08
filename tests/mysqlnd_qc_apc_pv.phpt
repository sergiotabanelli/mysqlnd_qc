--TEST--
Plain vanilla APC to check leaks...
--SKIPIF--
<?php
require_once('skipif.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_phpt"
--FILE--
<?php
	$myvar = 1;
	apc_store("myvar", $myvar);
 	unset($myvar);
	if (1 !== ($myvar = apc_fetch("myvar")))
		var_dump($myvar);
	apc_store("myvar", $myvar);
 	unset($myvar);
	if (1 !== ($myvar = apc_fetch("myvar")))
		var_dump($myvar);
	print "done!";
?>
--EXPECTF--
done!