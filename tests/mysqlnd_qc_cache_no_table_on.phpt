--TEST--
SLEEP() as an example of a non-cachable function but cached
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
mysqlnd_qc.cache_no_table=1
mysqlnd_qc.use_request_time=0
mysqlnd_qc.ttl=2
mysqlnd_qc.ignore_sql_comments=0
--FILE--
<?php
	require_once('handler.inc');
	require_once('connect.inc');

	if ("memcache" == $env_handler || (false == mysqlnd_qc_clear_cache())) {
		$query = randsql("SELECT SLEEP(1)");
	} else {
		$query = $QC_SQL_HINT . "SELECT SLEEP(1)";
	}
	printf("query = %s\n", $query);

	if (!$link = my_mysqli_connect($host, $user, $passwd, null, $port, $socket))
		printf("[001] [%d] %s\n", mysqli_connect_errno(), mysql_connect_error());

	$info = mysqlnd_qc_get_cache_info();
	$num_entries = $info['num_entries'];

	$start = microtime(true);
	if (!mysqli_query($link, $query))
		printf("[002] [%d] [%s]\n", mysqli_errno($link), mysqli_error($link));

	$runtime = microtime(true) - $start;
	if ($runtime < 1)
		printf("[003] Runtime of %2.2fs is suspicious, [%d] [%s]", $runtime, mysqli_errno($link), mysqli_error($link));

	/* query must be served from cache because of mysqlnd_qc.cache_no_table=1 */
	$start = microtime(true);
	if (!mysqli_query($link, $query))
		printf("[004] [%d] [%s]\n", mysqli_errno($link), mysqli_error($link));

	$runtime_cached = microtime(true) - $start;
	if ($runtime_cached > 1)
		printf("[005] Cached query took %2.2fs vs. %2.2fs uncached - very long!\n", $runtime_cached, $runtime);

	/* Some handler will not report the number of cache entries */
	$info = mysqlnd_qc_get_cache_info();
	if (!empty($info['data']) && ($num_entries + 1) != $info['num_entries'])
		printf("[006] Expecting %d cache entries found %d\n", $num_entries + 1, $info['num_entries']);


	mysqli_close($link);
	print "done!";
?>
--EXPECTF--
query = %s
done!