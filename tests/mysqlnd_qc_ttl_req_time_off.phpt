--TEST--
Simple TTL timeout
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
mysqlnd_qc.ttl=1
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	require("table.inc");

	$queries = array(
		$QC_SQL_HINT . "SELECT id FROM test WHERE id = 1" =>
			array('ttl' => 2, 'result' => array(), 'eol' => 0),
		$QC_SQL_HINT . "SELECT id FROM test WHERE id < 2 && id > 0" =>
			array('ttl' => 3, 'result' => array(), 'eol' => 0),
		$QC_SQL_HINT . "SELECT id FROM test WHERE id < 2"
			=> array('ttl' => 2, 'result' => array(), 'eol' => 0),
		$QC_SQL_HINT . "SELECT id FROM test WHERE id > 0 && id < 2"
			=> array('ttl' => 3, 'result' => array(), 'eol' => 0),
	);
	$queries_left = count($queries);

	$min_eol = PHP_INT_MAX;
	foreach ($queries as $query => $prop) {
		ini_set("mysqlnd_qc.ttl", $prop['ttl']);
		if (!$res = mysqli_query($link, $query)) {
			printf("[001] %s [%d] %s\n", $query, mysqli_errno($link), mysqli_error($link));
		} else {
			$queries[$query]['eol'] = ceil(microtime(true) + $prop['ttl']);
			if ($queries[$query]['eol'] < $min_eol)
				$min_eol = $queries[$query]['eol'];

			$queries[$query]['result'] = mysqli_fetch_all($res, MYSQLI_ASSOC);

			mysqli_free_result($res);
		}
	}

	if (!mysqli_query($link, "DELETE FROM test"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	for ($i = 0; $i < 1; $i++) {
		$min_eol = PHP_INT_MAX;
		foreach ($queries as $query => $prop) {
			if (!$res = mysqli_query($link, $query)) {
				printf("[003 + %d] %s [%d] %s\n", $i, $query, mysqli_errno($link), mysqli_error($link));
			} else {
				$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
				if (microtime(true) < $prop['eol']) {
					if ($rows != $prop['result']) {
						printf("[004 + %d] Wrong results for %s\n", $i, $query);
						var_dump($rows);
						var_dump($prop['result']);
					}
					if ($prop['eol'] < $min_eol)
						$min_eol = $prop['eol'];
				} else {
					/* expired */
					if (!empty($rows)) {
						printf("[005 + %d] Should be empty, wrong results for %s\n", $i, $query);
						var_dump($rows);
						var_dump($prop['result']);
					}
					$queries_left--;
				}
				mysqli_free_result($res);
			}
		}
	}

	assert($queries_left > 0);

	while ($queries_left > 0) {
		/* processing and think-time to avoid false positives */
		$usleep = ($min_eol - microtime(true) + 2) * 1000000;
		if ($usleep < 0)
			break;

		usleep($usleep);

		$min_eol = PHP_INT_MAX;
		foreach ($queries as $query => $prop) {
			if (!$res = mysqli_query($link, $query)) {
				printf("[006 + %d] %s [%d] %s\n", $queries_left, $query, mysqli_errno($link), mysqli_error($link));
			} else {
				$rows = mysqli_fetch_all($res, MYSQLI_ASSOC);
				$now = microtime(true);
				if ($now < $prop['eol']) {
					if ($rows != $prop['result']) {
						printf("[007 + %d] Wrong results for %s\n", $queries_left, $query);
						var_dump($rows);
						var_dump($prop['result']);
					}
					if ($prop['eol'] < $min_eol)
						$min_eol = $prop['eol'];
				} else {
					/* expired */
					if (!empty($rows)) {
						printf("[008 + %d] Should be empty, wrong results for %s\n", $queries_left, $query);
						printf("Expired at %f, now %f\n", $prop['eol'], $now);
						var_dump($rows);
						var_dump($prop['result']);
					}
					$queries_left--;
				}
				mysqli_free_result($res);
			}
		}
	}

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!