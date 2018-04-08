--TEST--
Impact of the query cache on uncached queries - skipped by default
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");

if (!isset($available_handler['apc']))
	die("SKIP APC handler not available");

die("SKIP performance testing only - needs to be enabled manually");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.slam_defense=0
apc.use_request_time=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_perf"
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
die("done!");
	require_once('handler.inc');
	require_once("connect.inc");

	/* use ~n bytes long simple SELECT <number>[,<number>...] queries */
	define("MAX_QUERY_LEN", 1024);
	/* more than 58 + 2 = 60 seconds may clash with the run-tests Skript */
	define("MAX_RUN_TIME", 50);
	set_time_limit(MAX_RUN_TIME + 2);

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[002] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	$run_time = 0;
	$total_len = 0;
	$num_queries = 0;
	$min_len = strlen("SELECT ");
	$max_len = MAX_QUERY_LEN - $min_len - 2;
	$wall_time_stop = microtime(true) + MAX_RUN_TIME;
	do {
		/* $max_len = mt_rand($min_len, MAX_QUERY_LEN) + 2

		$query = "SELECT ";
		do {
			$query .= sprintf("%d, ", mt_rand(1000000, 9999999));
		} while (strlen($query) < $max_len);
		$query = substr($query, 0, -2);
		*/
		$query = sprintf("SELECT '%s'", str_repeat((string)mt_rand(0, 9), $max_len));
		$total_len += strlen($query);
		$start = microtime(true);
		if (!$res = mysqli_query($link, $query)) {
			printf("[003] [%d] %s (%s)\n", mysqli_errno($link), mysqli_error($link), $query);
			break;
		}
		$run_time += (microtime(true) - $start);
		$row = mysqli_fetch_assoc($res);
		unset($row);
		mysqli_free_result($res);
		unset($res);
		$num_queries++;
	} while (microtime(true) < $wall_time_stop);

	printf("Number of queries: %d\n", $num_queries);
	printf("Total query string lenght in MB: %.2f\n", $total_len / 1024 / 1024);
	printf("Run time: %.6fs\n", $run_time);
	printf("Query length in kB / run time = %.2f kB/s\n", $total_len / $run_time / 1024);
	print "done!";
?>
--EXPECTF--
Some bogus to let the test fail but show you the results
Number of queries: %d
Total query string lenght in MB: %s
Run time: %s
Query length in MB / run time = %s
done!