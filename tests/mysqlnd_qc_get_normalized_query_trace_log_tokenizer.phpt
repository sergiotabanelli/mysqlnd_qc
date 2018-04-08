--TEST--
mysqlnd_qc_get_normalized_query_trace_log() - tokenizer stressing
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
mysqlnd_qc.collect_query_trace="0"
mysqlnd_qc.collect_normalized_query_trace="1"
mysqlnd_qc.query_trace_bt_depth=1
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require_once("handler.inc");

	function dump_trace($trace) {
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

	function query_and_dump($offset, $link, $query) {

		printf("[%03d + 01] %s\n", $offset, $query);
		if (!$res = mysqli_query($link, $query)) {
			printf("[%03d + 02] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}

		$data = mysqli_fetch_all($res, MYSQLI_ASSOC);
		mysqli_free_result($res);
		if (empty($data)) {
			printf("[%03d + 03] No data, [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return false;
		}

		$trace = mysqlnd_qc_get_normalized_query_trace_log();
		printf("%-30s: %d\n", "Trace entries", count($trace));
/*
		foreach ($trace as $k => $v) {
			printf("%-30s: %d - %s\n", "Query", $k, $v['query']);
		}
*/
		dump_trace($trace[count($trace) - 1]);

		return true;
	}

	require_once("connect.inc");
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	query_and_dump(2, $link, $QC_SQL_HINT. "SELECT 1, '?', '\''");
	query_and_dump(3, $link, $QC_SQL_HINT. "SELECT '\0'");
	query_and_dump(4, $link, $QC_SQL_HINT. "SELECT '". chr(0). "'");
	query_and_dump(5, $link, $QC_SQL_HINT. "SELECT 1, '". chr(0). "', 2");
	query_and_dump(6, $link, $QC_SQL_HINT. "SELECT 1, '". chr(0). "', '\''");
	query_and_dump(7, $link, $QC_SQL_HINT. "SELECT 1 AS 'one', '". chr(0). "', '\'' FROM DUAL");

	if (!mysqli_query($link, "DROP TABLE IF EXISTS test") ||
		!mysqli_query($link, "CREATE TABLE test(id INT, label CHAR(10)) ENGINE = ". $engine) ||
		!mysqli_query($link, "INSERT INTO test(id, label) VALUES (1, 'a'), (2, 'b'), (3, 'c')"))
		printf("[008] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	query_and_dump(9, $link, $QC_SQL_HINT. "SELECT id FROM test WHERE id = 1");
	query_and_dump(10, $link, $QC_SQL_HINT. "SELECT id FROM test WHERE id = 1 OR label = '". chr(0). "'");
	query_and_dump(11, $link, $QC_SQL_HINT. "SELECT id FROM test WHERE id = 1 OR label = '". chr(0). "' ORDER BY id ASC");
	query_and_dump(12, $link, $QC_SQL_HINT. "SELECT id FROM test WHERE id = 1 OR label = '". chr(0). "' ORDER BY id ASC LIMIT 5");

	query_and_dump(13, $link, $QC_SQL_HINT. "SELECT t1.id FROM test AS t1, test AS t2 WHERE t1.id = 1 OR t1.label = '". chr(0). "' ORDER BY t1.id ASC LIMIT 5");
	query_and_dump(14, $link, $QC_SQL_HINT. "SELECT t1.id FROM test AS t1, test AS t2 WHERE t1.id = 1 OR t1.label = '". chr(0). "' ORDER BY t1.id ASC LIMIT 5");
	query_and_dump(15, $link, $QC_SQL_HINT. "SELECT 1, '?', '\''");

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
[002 + 01] /*%s*/%s
Trace entries                 : 1
query                         : s(SELECT ?, ?, ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[003 + 01] /*%s*/%s
Trace entries                 : 2
query                         : s(SELECT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[004 + 01] /*%s*/%s
Trace entries                 : 2
query                         : s(SELECT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

[005 + 01] /*%s*/%s
Trace entries                 : 2
query                         : s(SELECT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

[006 + 01] /*%s*/%s
Trace entries                 : 2
query                         : s(SELECT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

[007 + 01] /*%s*/%s
Trace entries                 : 3
query                         : s(SELECT ? AS ?, ?, ? FROM DUAL)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[009 + 01] /*%s*/%s
Trace entries                 : 7
query                         : s(SELECT id FROM test WHERE id =?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[010 + 01] /*%s*/%s
Trace entries                 : 8
query                         : s(SELECT id FROM test WHERE id =? OR label =?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[011 + 01] /*%s*/%s
Trace entries                 : 9
query                         : s(SELECT id FROM test WHERE id =? OR label =? ORDER BY id ASC)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[012 + 01] /*%s*/%s
Trace entries                 : 10
query                         : s(SELECT id FROM test WHERE id =? OR label =? ORDER BY id ASC LIMIT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[013 + 01] /*%s*/%s
Trace entries                 : 11
query                         : s(SELECT t1.id FROM test AS t1, test AS t2 WHERE t1.id =? OR t1.label =? ORDER BY t1.id ASC LIMIT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(1)

[014 + 01] /*%s*/%s
Trace entries                 : 11
query                         : s(SELECT t1.id FROM test AS t1, test AS t2 WHERE t1.id =? OR t1.label =? ORDER BY t1.id ASC LIMIT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

[015 + 01] /*%s*/%s
Trace entries                 : 11
query                         : s(SELECT t1.id FROM test AS t1, test AS t2 WHERE t1.id =? OR t1.label =? ORDER BY t1.id ASC LIMIT ?)
min_run_time                  : i(%d)
avg_run_time                  : i(%d)
max_run_time                  : i(%d)
min_store_time                : i(%d)
avg_store_time                : i(%d)
max_store_time                : i(%d)
eligible_for_caching          : b(true)
occurences                    : i(2)

done!