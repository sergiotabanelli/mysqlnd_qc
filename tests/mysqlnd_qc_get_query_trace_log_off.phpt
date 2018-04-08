--TEST--
mysqlnd_qc_get_[normalized]_query_trace_log(), mysqlnd_qc.collect_query_trace="0"
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
mysqlnd_qc.time_statistics=1;
mysqlnd_qc.collect_statistics=1
mysqlnd_qc.collect_query_trace="0"
mysqlnd_qc.collect_normalized_query_trace="0"
mysqlnd_qc.query_trace_bt_depth=1
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require_once("handler.inc");
	require_once("table.inc");
	var_dump(mysqlnd_qc_get_query_trace_log());
	var_dump(mysqlnd_qc_get_normalized_query_trace_log());

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
array(0) {
}
array(0) {
}
done!