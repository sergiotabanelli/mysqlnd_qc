--TEST--
mysqlnd_qc_get_query_trace_log() and INSERT
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
mysqlnd_qc.time_statistics=1;
mysqlnd_qc.collect_statistics=1
mysqlnd_qc.collect_query_trace="1"
mysqlnd_qc.collect_normalized_query_trace="1"
mysqlnd_qc.query_trace_bt_depth=3
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

	$sql = <<<EOT
DROP TABLE IF EXISTS `test`;
CREATE TABLE `test` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `fk_statistics_record_id` int(10) unsigned NOT NULL,
  `query_string` varchar(1024) DEFAULT NULL,
  `origin` blob,
  `run_time` int(10) unsigned DEFAULT NULL,
  `store_time` int(10) unsigned DEFAULT NULL,
  `eligible_for_caching` enum('true','false') DEFAULT NULL,
  `no_table` enum('true','false') DEFAULT NULL,
  `was_added` enum('true','false') DEFAULT NULL,
  `was_already_in_cache` enum('true','false') DEFAULT NULL,
  PRIMARY KEY (`id`)
);
EOT;

	if (!mysqli_multi_query($link, $sql))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	clear_line($link);

	$trace = mysqlnd_qc_get_normalized_query_trace_log();
	dump_trace($trace);

	$query_log = mysqlnd_qc_get_query_trace_log();
	foreach ($query_log as $k => $bt) {
;
$sql =<<<EOT
                        INSERT INTO test(
                                fk_statistics_record_id,
                                query_string,
                                origin,
                                run_time,
                                store_time,
                                eligible_for_caching,
                                no_table,
                                was_added,
                                was_already_in_cache
                        ) VALUES (
                                0,
                                'DROP TABLE IF EXISTS test',
                                '#0 /home/nixnutz/src/php-mysqlnd-qc-bzr/trunk/tests/table.inc(10): mysqli_query(Object(mysqli), \'DROP TABLE IF E...\')\n#1 /home/nixnutz/src/php-mysqlnd-qc-bzr/trunk/web/auto_append_persist_qc_stats.php(35): require_once(\'/home/nixnutz/s...\')\n#2 {main}',
                                0,
                                0,
                                'false',
                                'false',
                                'false',
                                'false'
                        )
EOT;
			if (!$link->query($sql))
				printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
		}

	var_dump(mysqlnd_qc_get_normalized_query_trace_log());
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
array(1) {
  [0]=>
  array(9) {
    ["query"]=>
    string(188) "INSERT INTO test (fk_statistics_record_id, query_string, origin, run_time, store_time, eligible_for_caching, no_table, was_added, was_already_in_cache ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ? )"
    ["occurences"]=>
    int(0)
    ["eligible_for_caching"]=>
    bool(false)
    ["avg_run_time"]=>
    int(0)
    ["min_run_time"]=>
    int(0)
    ["max_run_time"]=>
    int(0)
    ["avg_store_time"]=>
    int(0)
    ["min_store_time"]=>
    int(0)
    ["max_store_time"]=>
    int(0)
  }
}
done!