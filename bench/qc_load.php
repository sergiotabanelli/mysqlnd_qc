<?php
/**
* Basic script to test the stability of the QC under concurrent load
*
* The script runs a list of cached queries in a loop for the configured duration
* (default: 0.5s) and compares the cached results with uncached ones. At the end
* of the test run the script writes the number of completed queries into the DB
* table "results".
*
* Installation and usage:
*
*   1) Upload the script to your web server
*   2) Set the configuration variables on top of the script - search for "CONFIGURE"
*   3) Setup the database by invoking http://<myserver>/qc_load.php?init=1
*   4) Run the script, e.g. using ab -n 5000 -c 20 http://<myserver>/qc_load.php (~5 min on 2.8 GHz Dual-Core Intel *)
*   5) Check the results:
*   6) If the database shows MIN(num_queries) = 0 the cache has failed and has delivered wrong results!
*   7) Study the results: SELECT handler, handler_version, MIN(num_queries), AVG(num_queries), MAX(num_queries) FROM results GROUP BY CONCAT(handler, handler_version);
*   8) Ensure that all entries from test_runs have the status "completed" - if not, bug!  SELECT handler, handler_version, status FROM test_runs GROUP BY CONCAT(handler, handler_version, status);
*
* Benchmarking:
*
* You can "abuse" the script to compare the performance of the various storage handler.
* Just benchmark with different settings. Once you are done, check results recorded in the DB.
*
* Database tables:
*
* test1 - Garbage table to fetch from to generate some load
* test2- Garbage table to fetch from to generate some load
* test_runs - Table which records the start and stop of each invocation of the test script
* results - Table which shows the number of served queries for each successful invocation of the test script
*
* Yeah, test_runs and results should have been one table. I don't recally why I have used two tables.
* The only purpose of the test_runs table is to check that all no crashes have happened. Yeah, it could
* have done in one table - I already said that...
*/


/* CONFIGURE - Storage handler to use for testing: "user", "apc", "memcache", "default", "noqc", "nocache", "sqlite" */
$env_handler = "default";
/* CONFIGURE - Number of rows in the example table - total resultset size ~ $rows * 280 bytes */
$rows = 100;
/* CONFIGURE - How long (seconds) to run queries before the script terminates */
$runtime = 20;
/* CONFIGURE */
$QC_SQL_HINT = "/*qc=on*/";
/* CONFIGURE - collect statistics */
$collect_stats = true;

ini_set("max_run_time", $runtime + 2);

function my_mysql_connect() {
	/* CONFIGURE - Database connection parameter */
	$host = 'localhost';
	$db = 'test';
	$user = 'root';
	$pass = 'root';
	$port = 0;
	$socket = '/tmp/mysql.sock';

	return mysqli_connect($host, $user, $pass, $db, $port, $socket);
}

/* MAIN */
if (isset($_GET['handler']))
	$env_handler = $_GET['handler'];

$env_handler = strtolower($env_handler);
$have_qc = true;

switch ($env_handler) {
	case "memcache":
	case "apc":
	case "sqlite":
		mysqlnd_qc_change_handler($env_handler);
		break;
	case "user":
		mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "is_select", "update_stats", "clear_cache");
		ini_set("memory_limit", "256M");
		break;
	case "default":
	case "nocache":
		break;
	case "noqc":
		/* No query cache at all - we want to compare PHP with and wo QC enabled */
		$have_qc = false;
		break;
	default:
		die("Unknown handler $env_handler");
		break;
}


if ($have_qc) {
	$info = mysqlnd_qc_get_cache_info();
} else {
	$info = array('handler' => $env_handler, 'handler_version' => "1.0.0");
}
$handler = ('nocache' == $env_handler) ? "nocache" : $info['handler'];
$handler_version = ('nocache' == $env_handler) ? "" : $info['handler_version'];
$link = my_mysql_connect();

if (isset($_GET['init'])) {
	init_db($link);
	mysqli_close($link);
	die(sprintf("Database tables have been set up. You can start the benchmark.\n"));
}

if (isset($_GET['show'])) {
	show_results($link, $handler, $handler_version);
	die(sprintf("See above for results.\n"));
}

if ($have_qc && $collect_stats) {
	ini_set('mysqlnd_qc.collect_statistics', 1);
	ini_set('mysqlnd_qc.time_statistics', 1);
}

if (!mysqli_query($link, sprintf("
		INSERT INTO test_runs
			(
				pid, handler, handler_version, status
			) VALUES (
				%d, '%s', '%s', 'started'
			)", getmypid(), $handler, $handler_version))) {
	printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	die(":-(");
}
$test_run_id = mysqli_insert_id($link);

$queries = array(
	/* Of course, the cache efficiency will depend on your queries... */
	array("SELECT * FROM test1", true),
	array("SELECT * FROM test2 ORDER BY id ASC", true),
	array("SELECT * FROM test1 ORDER BY id ASC LIMIT 10", true),
	array("SELECT label FROM test2 WHERE id > 10", true),
);

$start = microtime(true);
$results = array();
$num_queries = 0;
do {
	foreach ($queries as $k => $query_details) {
		if (!$res = mysqli_query($link, $query_details[0])) {
			printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
			break 2;
		}
		if ($query_details[1]) {
			/* first round: uncached */
			$queries[$k][0] = (($env_handler == "nocache") ? "" : $QC_SQL_HINT) . $query_details[0];
			$queries[$k][1] = false;
		}
		$num_queries++;
		if (!isset($results[$k])) {
			$results[$k] =  mysqli_fetch_all($res, MYSQLI_ASSOC);
		} else {
			$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
			if ($rows != $results[$k]) {
				printf("[003] WRONG RESULTS\n");
				var_dump($rows);
				var_dump($results[$k]);
				$num_queries = 0;
				break 2;
			}
		}
		mysqli_free_result($res);
	}
	$now = microtime(true);
} while (($now - $start) < $runtime);

printf("Done: %s %s - %d queries\n", $handler, $handler_version, $num_queries);
if ($have_qc) {
	$stats = mysqlnd_qc_get_core_stats();
} else {
	$stats = array(
		'cache_put' 							=> 0,
		'cache_miss'							=> 0,
		'query_not_cached' 				=> 0,
		'query_could_cache'				=> 0,
		'query_uncached_other'		=> 0,
		'query_uncached_no_table'	=> 0,
		'query_uncached_no_result'=> 0,
		'query_aggr_run_time'			=> 0,
		'query_aggr_store_time'		=> 0,

	);
}

if (!mysqli_query($link, sprintf("
	INSERT INTO results
		(
			num_queries, handler, handler_version, fk_test_run_id,
			st_cache_put, st_cache_miss,	st_cache_hit,
			st_query_not_cached, st_query_could_cache,
			st_query_uncached_other, st_query_uncached_no_table,
			st_query_uncached_no_result,
			st_query_aggr_run_time,	st_query_aggr_store_time
		)
			VALUES
		(
			%d, '%s', '%s', %d,
			%d, %d, %d,
			%d, %d,
			%d, %d,
			%d, %d,
			%d
		)",
				$num_queries, $handler, $handler_version, $test_run_id,
				$stats['cache_put'], $stats['cache_miss'], $stats['cache_hit'],
				$stats['query_not_cached'], $stats['query_could_cache'],
				$stats['query_uncached_other'], $stats['query_uncached_no_table'],
				$stats['query_uncached_no_result'],
				$stats['query_aggr_run_time'], $stats['query_aggr_store_time']
				)))
	printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

if (!mysqli_query($link, sprintf("UPDATE test_runs SET status = 'completed' WHERE id = %d", $test_run_id)))
	printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

show_results($link, $handler, $handler_version);

mysqli_close($link);




/* Functions */

function show_results($link, $test_handler, $test_handler_version) {

	if (!$res = mysqli_query($link, "
			SELECT
				handler,
				handler_version,
				COUNT(*) as _num_test_runs,
				MIN(num_queries) AS _min,
				AVG(num_queries) AS _avg,
				MAX(num_queries) AS _max,
				AVG(st_cache_put) AS _cache_put,
				AVG(st_cache_miss) AS _cache_miss,
				AVG(st_cache_hit) AS _cache_hit,
				AVG(st_query_not_cached) AS _query_not_cached,
				AVG(st_query_could_cache) AS _query_could_cache,
				AVG(st_query_uncached_other) AS _query_uncached_other,
				AVG(st_query_uncached_no_table) AS _query_uncached_no_table,
				AVG(st_query_uncached_no_result) AS _query_uncached_no_result,
				AVG(st_query_aggr_run_time) AS _query_aggr_run_time,
				AVG(st_query_aggr_store_time) AS _query_aggr_store_time
			FROM
				results
			GROUP BY
				CONCAT(handler, handler_version)
			ORDER BY
				handler, handler_version, AVG(num_queries) ASC"))
		printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));

	?>
	<h3>Results</h3>
	<p>
		Active handler: <?php print $test_handler; ?>, <?php print $test_handler_version; ?>
	</p>
	<p>
	<table border="1" cellspacing="2" cellpadding="2">
		<tr bgcolor="#e0e0e0">
			<th>Handler</th>
			<th>Version</th>
			<th>Number of test runs</th>
			<th>MIN(num_queries)</th>
			<th>AVG(num_queries)</th>
			<th>MAX(num_queries)</th>
			<th>AVG(st_cache_put)</th>
			<th>AVG(st_cache_miss)</th>
			<th>AVG(st_cache_hit)</th>
			<th>AVG(st_query_not_cached)</th>
			<th>AVG(st_query_could_cache)</th>
			<th>AVG(st_query_uncached_other)</th>
			<th>AVG(st_query_uncached_no_table)</th>
			<th>AVG(st_query_uncached_no_result)</th>
			<th>AVG(st_query_aggr_run_time)</th>
			<th>AVG(st_query_aggr_store_time)</th>
		</tr>
	<?php
	$fac_min = $fac_avg = $fac_max = NULL;
	while ($row = mysqli_fetch_assoc($res)) {
		if ($fac_min == NULL) {
			$fac_min = ($row['_min'] > 0) ? 100 / $row['_min'] : 0;
			$fac_avg = ($row['_avg'] > 0) ? 100 / $row['_avg'] : 0;
			$fac_max = ($row['_max'] > 0) ? 100 / $row['_max'] : 0;
		}
		?>
		<tr>
			<td><?php print htmlspecialchars($row['handler']); ?></td>
			<td><?php print htmlspecialchars($row['handler_version']); ?></td>
			<td align="right"><?php printf("%d", $row['_num_test_runs']); ?></td>
			<td align="right"><?php printf("%d (%d %%)", $row['_min'], $fac_min * $row['_min']); ?></td>
			<td align="right"><?php printf("%d (%d %%)", $row['_avg'], $fac_avg * $row['_avg']); ?></td>
			<td align="right"><?php printf("%d (%d %%)", $row['_max'], $fac_max * $row['_max']); ?></td>
			<td><?php print htmlspecialchars($row['_cache_put']); ?></td>
			<td><?php print htmlspecialchars($row['_cache_miss']); ?></td>
			<td><?php print htmlspecialchars($row['_cache_hit']); ?></td>
			<td><?php print htmlspecialchars($row['_query_not_cached']); ?></td>
			<td><?php print htmlspecialchars($row['_query_could_cache']); ?></td>
			<td><?php print htmlspecialchars($row['_query_uncached_other']); ?></td>
			<td><?php print htmlspecialchars($row['_query_uncached_no_table']); ?></td>
			<td><?php print htmlspecialchars($row['_query_uncached_no_result']); ?></td>
			<td><?php print htmlspecialchars($row['_query_aggr_run_time']); ?></td>
			<td><?php print htmlspecialchars($row['_query_aggr_store_time']); ?></td>
		</tr>
		<?php
	} // end while ($row = mysqli_fetch_assoc($res)) {
	?>
	</table>
	</p>
	<?php
}

function init_db($link) {

	if (!mysqli_query($link, "DROP TABLE IF EXISTS test1"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_query($link, "CREATE TABLE test1(id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, label varchar(255))"))
		printf("[007] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$label = str_repeat('a', 255);
	$query = sprintf("INSERT INTO test1(label) VALUES ('%s')", mysqli_real_escape_string($link, $label));
	for ($i = 0; $i < $rows; $i++) {
		if (!mysqli_query($link, $query))
			printf("[008] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	if (!mysqli_query($link, "DROP TABLE IF EXISTS test2"))
		printf("[009] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_query($link, "CREATE TABLE test2(id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, label varchar(255))"))
		printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$label = str_repeat('a', 255);
	$query = sprintf("INSERT INTO test2(label) VALUES ('%s')", mysqli_real_escape_string($link, $label));
	for ($i = 0; $i < $rows; $i++) {
		if (!mysqli_query($link, $query))
			printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	if (!mysqli_query($link, "DROP TABLE IF EXISTS test_runs"))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

if (!mysqli_query($link, "
	CREATE TABLE test_runs(
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		handler VARCHAR(127),
		handler_version VARCHAR(127),
		pid INT, status ENUM('started','completed') NOT NULL DEFAULT 'started',
		st_cache_put BIGINT UNSIGNED,
		st_cache_miss BIGINT UNSIGNED,
		st_cache_hit BIGINT UNSIGNED,
		st_query_not_cached BIGINT UNSIGNED,
		st_query_could_cache BIGINT UNSIGNED,
		st_query_uncached_other BIGINT UNSIGNED,
		st_query_uncached_no_table BIGINT UNSIGNED,
		st_query_uncached_no_result BIGINT UNSIGNED,
		st_query_aggr_run_time BIGINT UNSIGNED,
		st_query_aggr_store_time BIGINT UNSIGNED
	)"))
		printf("[013] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_query($link, "DROP TABLE IF EXISTS results"))
		printf("[014] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_query($link, "CREATE TABLE results(
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		handler VARCHAR(127),
		handler_version VARCHAR(127),
		num_queries INT,
		fk_test_run_id INT,
		st_cache_put BIGINT UNSIGNED,
		st_cache_miss BIGINT UNSIGNED,
		st_cache_hit BIGINT UNSIGNED,
		st_query_not_cached BIGINT UNSIGNED,
		st_query_could_cache BIGINT UNSIGNED,
		st_query_uncached_other BIGINT UNSIGNED,
		st_query_uncached_no_table BIGINT UNSIGNED,
		st_query_uncached_no_result BIGINT UNSIGNED,
		st_query_aggr_run_time BIGINT UNSIGNED,
		st_query_aggr_store_time BIGINT UNSIGNED
	)"))
		printf("[015] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

}

/* A couple of plain vanilly user handler callback functions */

function clear_cache_void() {
	global $__cache;
}

function clear_cache() {
	global $__cache;
	$__cache = array();
	return true;
}

function get_hash($host_info, $port, $user, $db, $query) {
	global $__cache, $verbose;
	if ($verbose)
		printf("get_hash('%s', '%s', '%s', '%s', '%s')\n", $host_info, $port, $user, $db, $query);

	$ret = md5($host_info . $port .  $user . $db . $query);
	if ($verbose)
		printf("\tret = '%s'\n", $ret);

	return $ret;
}

function get_hash_void($host_info, $port, $user, $db, $query) {
	global $verbose;
	if ($verbose)
		printf("get_hash_void() proxy\n");
	get_hash($host_info, $port, $user, $db, $query);
}

function find($key) {
	global $__cache, $verbose;
	if ($verbose)
		printf("find('%s')\n", $key);

	if (isset($__cache[$key])) {
		$ret = $__cache[$key]["data"];
		$__cache[$key]["hits"]++;
	} else {
		$ret = NULL;
	}

	if ($verbose)
		printf("\tret = %s[%d]\n", gettype($ret), strlen($ret));

	return $ret;
}

function find_void($key) {
	global $verbose;
	if ($verbose)
		printf("find_void() proxy\n");
	find($key);
}

function add($key, $data, $ttl, $runtime, $store_time, $row_count) {
	global $__cache, $verbose;
	if ($verbose)
		printf("add('%s', %s[%d], %d, %.4f, %.4f, %d)\n", $key, gettype($data), strlen($data), $ttl, $runtime, $store_time, $row_count);

	$__cache[$key] = array("data" => $data, "hits" => 0, "run_time" => array(), "store_time" => array());
	return true;
}

function add_void($key, $data, $ttl, $runtime, $store_time, $row_count) {
	global $verbose;
	if ($verbose)
		printf("add_void()\n");
}

function is_select($query) {
	global $QC_SQL_HINT, $verbose;

	if ($verbose)
		printf("is_select('%s')\n", $query);
	$ret = stristr($query, $QC_SQL_HINT);
	if ($verbose)
		printf("\tret = %s\n", var_export($ret, true));

	return $ret;
}

function is_select_void($query) {
	global $verbose;
	if ($verbose)
		printf("is_select_void() proxy\n");
	is_select($query);
}

function return_to_cache($key) {
	global $verbose;
	if ($verbose)
		printf("return_to_cache('%s')\n", $key);
}

function update_stats($key, $run_time, $store_time) {
	global $verbose, $__cache;;

	if ($verbose)
		printf("update_stats(%s)\n", var_export(func_get_args(), true));

	if (isset($__cache[$key])) {
		$__cache[$key]["run_time"][] = $run_time;
		$__cache[$key]["store_time"][] = $store_time;
	}
}

function dummy() {
	global $verbose;
	if ($verbose)
		printf("dummy(%s)\n", var_export(func_get_args(), true));
}

function get_stats($key = NULL) {
	global $__cache;
	$stats = array();
	if (is_null($key)) {
		foreach ($__cache as $key => $details) {
			$stats[$key] = array('run_time' => $details['run_time'], 'store_time' => $details['store_time']);
		}
	} else {
		if (isset($__cache[$key]))
			$stats[$key] = array('run_time' => $__cache[$key]['run_time'], 'store_time' => $__cache[$key]['store_time']);
	}
	return $stats;
}
?>