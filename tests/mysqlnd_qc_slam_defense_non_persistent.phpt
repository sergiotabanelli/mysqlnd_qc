--TEST--
Slam defense for non-persistent storage
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.slam_defense=1
mysqlnd_qc.cache_no_table=1
mysqlnd_qc.ttl=1
--SKIPIF--
<?php
die("skip Test is not functional");
require_once('skipif.inc');
require_once("connect.inc");
require("handler.inc");
if (("default" !== $env_handler) && ("user" !== $env_handler))
	die("skip Slam defense is not available with handler " . $env_handler);

if (!$IS_MYSQLND)
	die("skip mysqlnd only feature, compile PHP using --with-mysqli=mysqlnd");
?>
--FILE--
<?php
	require("handler.inc");
	require("table.inc");
	$verbose = true;

	function get_link() {
		global $host, $user, $passwd, $db, $port, $socket;
		if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
			printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
		}
		return $link;
	}
	function run_time() {
		global $start;
		return microtime(true) - $start;
	}

	/* fill cache */
	$start = microtime(true);
	printf("%3.2f %-20s\n", run_time(), "start");

	$ttl = ini_get('mysqlnd_qc.ttl');
	$sleep = $ttl + 1;
	/* query will be cached but expire immediately because of SLEEP(ttl + 1) */
	$select = sprintf("%sSELECT id, label, SLEEP(%d) AS _sleep FROM test WHERE id = 1", $QC_SQL_HINT, $sleep);

	if (!$res = mysqli_query($link, $select))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	mysqli_close($link);
	printf("%3.2f %-20s\n", run_time(), "Cache filled");

	while (run_time() < $ttl)
		sleep(1);

	printf("%3.2f %-20s\n", run_time(), "Stale fetch begins");
	$stats = mysqlnd_qc_get_core_stats();
	$links = array();
	for ($i = 0; $i < 4; $i++) {
		$link = get_link();

		if (true !== ($tmp = mysqli_query($link, $select, MYSQLI_ASYNC)))
			printf("[003] Expecting true got %s/%s\n", gettype($tmp), var_export($tmp, true));

		/* 0.1s to let the cache detect the slam */
		usleep(300000);

		$links[mysqli_thread_id($link)] = array(
			'query' 	=> $select,
			'link' 		=> $link,
			'processed' => false,
		);
	}
	printf("%3.2f %-20s\n", run_time(), "Stale fetch started");

	do {
		$poll_links = $poll_errors = $poll_reject = array();
		foreach ($links as $thread_id => $link) {
			if (!$link['processed']) {
				$poll_links[] 	= $link['link'];
				$poll_errors[]	= $link['link'];
				$poll_reject[]	= $link['link'];
			}
		}
		if (0 == count($poll_links))
			break;

		if (0 == ($num_ready = mysqli_poll($poll_links, $poll_errors, $poll_reject, 0, 200000)))
			continue;

		if (!empty($poll_errors)) {
			die(var_dump($poll_errors));
		}

		foreach ($poll_links as $link) {
			$thread_id = mysqli_thread_id($link);
			$links[$thread_id]['processed'] = true;

			if (is_object($res = mysqli_reap_async_query($link))) {
				// result set object
				printf("%3.2f %-20s: %d\n", run_time(), "Fetch", $thread_id);
				$rown = mysqli_fetch_assoc($res);
				mysqli_free_result($res);
				if ($row != $rown) {
					printf("[004] Wrong results, dumping data\n");
					var_dump($row);
					var_dump($rown);
				}
			} else {
				// either there is no result (no SELECT) or there is an error
				if (mysqli_errno($link) > 0) {
					$saved_errors[$thread_id] = mysqli_errno($link);
					printf("[005] '%s' caused %d\n", $links[$thread_id]['query'],	mysqli_errno($link));
				}
			}
		}

	} while (true);
	printf("%3.2f %-20s\n", run_time(), "Stale fetch done");

	$statsn = mysqlnd_qc_get_core_stats();
	var_dump($stats);
	var_dump($statsn);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!