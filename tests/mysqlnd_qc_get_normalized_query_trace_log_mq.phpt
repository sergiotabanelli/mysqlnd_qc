--TEST--
mysqlnd_qc_get_query_trace_log() and multi_query
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
require_once("handler.inc");
if ("userp" == $env_handler)
	die("skip Not supported with handler");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.time_statistics=0;
mysqlnd_qc.collect_statistics=0
mysqlnd_qc.collect_query_trace="0"
mysqlnd_qc.collect_normalized_query_trace="1"
mysqlnd_qc.query_trace_bt_depth=1
mysqlnd_qc.cache_no_table=1
mysqlnd_qc.cache_by_default=0
--FILE--
<?php
	require_once("handler.inc");
	require_once("connect.inc");

	function dump_trace($trace_list) {
		$expected = array(
			'query' 					=> 'string',
			'min_run_time'				=> 'int',
			'avg_run_time'				=> 'int',
			'max_run_time'				=> 'int',
			'min_store_time'			=> 'int',
			'avg_store_time'			=> 'int',
			'max_store_time'			=> 'int',
			'eligible_for_caching' 		=> 'bool',
			'occurences'				=> 'int',
		);

		foreach ($trace_list as $k => $trace) {
			foreach ($expected as $field => $type) {
				switch ($type) {
					case 'int':
						printf("%-30s: i(%d)\n", $field, $trace[$field]);
						break;
					case 'bool':
						printf("%-30s: b(%s)\n", $field, $trace[$field] ? 'true' : 'false');
						break;
					case 'string':
					default:
						printf("%-30s: s(%s)\n", $field, $trace[$field]);
						break;
				}
				unset($trace[$field]);
			}
			printf("\n");

			if (count($trace)) {
				printf("Unexpected columns! You need to update the test.");
				var_dump($trace);
			}
		}
	}

	function clear_line($link) {
		while (mysqli_more_results($link) && mysqli_next_result($link) && ($res = mysqli_store_result($link))) {
			while ($row = mysqli_fetch_array($res))
				;
			mysqli_free_result($res);
		}
	}

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	if (!mysqli_multi_query($link, "DROP TABLE IF EXISTS test; CREATE TABLE test(id INT)"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	clear_line($link);

	if (!mysqli_multi_query($link, "INSERT INTO test(id) VALUES (1); INSERT INTO test(id) VALUES (2);"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	clear_line($link);

	/* single query but MQ API - shall be filtered out */
	if (!mysqli_multi_query($link, "INSERT INTO test(id) VALUES (3)"))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	clear_line($link);

	if (!$res = mysqli_query($link, "SELECT id FROM test ORDER BY id ASC"))
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	while ($row = mysqli_fetch_assoc($res))
		printf("id = %d\n", $row['id']);
	mysqli_free_result($res);

	if (!$res = mysqli_query($link, "DROP TABLE IF EXISTS test"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$trace = mysqlnd_qc_get_normalized_query_trace_log();
	dump_trace($trace);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
id = 1
id = 2
id = 3
query                         : s(SELECT id FROM test ORDER BY id ASC)
min_run_time                  : i(0)
avg_run_time                  : i(0)
max_run_time                  : i(0)
min_store_time                : i(0)
avg_store_time                : i(0)
max_store_time                : i(0)
eligible_for_caching          : b(false)
occurences                    : i(0)

query                         : s(DROP TABLE IF EXISTS test)
min_run_time                  : i(0)
avg_run_time                  : i(0)
max_run_time                  : i(0)
min_store_time                : i(0)
avg_store_time                : i(0)
max_store_time                : i(0)
eligible_for_caching          : b(false)
occurences                    : i(0)

done!