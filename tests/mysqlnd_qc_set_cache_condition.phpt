--TEST--
mysqlnd_qc_set_cache_condition()
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
mysqlnd_qc.collect_statistics=1
--FILE--
<?php
	require_once("handler.inc");
	require_once("table.inc");

	function dump_put_hit() {
		$stats = mysqlnd_qc_get_core_stats();
		printf("cache_put %d\n", $stats['cache_put']);
		printf("cache_hit %d\n", $stats['cache_hit']);
	}

	var_dump(mysqlnd_qc_set_cache_condition(MYSQLND_QC_CONDITION_META_SCHEMA_PATTERN, $db . ".%", 1));
	var_dump(mysqlnd_qc_set_cache_condition(MYSQLND_QC_CONDITION_META_SCHEMA_PATTERN, "asdhjawhudihui.%", 1));

	$query = "SELECT id FROM test";
	var_dump($link->query($query)->fetch_assoc());;
	dump_put_hit();
	usleep(100);
	var_dump($link->query($query)->fetch_assoc());;
	dump_put_hit();

	var_dump($link->query("DELETE FROM test"));
	usleep(100);
	var_dump($link->query($query)->fetch_assoc());;
	dump_put_hit();

	if (NULL !== ($tmp = @mysqlnd_qc_set_cache_condition("too many args")))
		printf("[001] Expecting NULL got %s\n", var_export($tmp, true));

	print "done!";
?>
--CLEAN--
<php  require_once("clean_table.inc"); ?>
--EXPECTF--
bool(true)
bool(true)
array(1) {
  ["id"]=>
  string(1) "1"
}
cache_put 1
cache_hit 0
array(1) {
  ["id"]=>
  string(1) "1"
}
cache_put 1
cache_hit 1
bool(true)
array(1) {
  ["id"]=>
  string(1) "1"
}
cache_put 1
cache_hit 2
done!