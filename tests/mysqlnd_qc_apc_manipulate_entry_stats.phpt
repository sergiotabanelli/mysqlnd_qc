--TEST--
APC handler - manipulate cache entries and check cache info
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
if (!extension_loaded("APC"))
	die("SKIP APC not available");

if (!isset($available_handler['apc']))
	die("SKIP APC handler not available");
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_manip_phpt"
mysqlnd_qc.cache_no_table=0
--FILE--
<?php
	function find_cache_entry($query) {
		global $QC_SQL_HINT;

		$prefix = ini_get('mysqlnd_qc.apc_prefix');
		$prefix_len = strlen($prefix);

		$cached_query = false;
		$key = '';

		if ($QC_SQL_HINT == substr($query, 0, strlen($QC_SQL_HINT))) {
			$query = substr($query, strlen($QC_SQL_HINT), strlen($query));
			$query = preg_replace("/\s+/", "\\s*", preg_quote($query));
		}

		$user = apc_cache_info("user");
		foreach ($user['cache_list'] as $data) {
			if ((substr($data['info'], 0, $prefix_len) == $prefix)
				&& (preg_match("|" . $query . "|", $data['info']))) {
				$cached_query = apc_fetch($data['info']);
				$key = $data['info'];
				break;
			}
		}

		$tmp = array($key => $cached_query);
		return $tmp;
	}

	function run_query($offset, $link, $query) {
		$ret = NULL;

		if (!$res = mysqli_query($link, $query)) {
			printf("[%03d + 1] %s - [%d] %s\n", $offset, $query, mysqli_errno($link), mysqli_error($link));
			return $ret;
		}
		$ret = mysqli_fetch_all($res, MYSQLI_ASSOC);
		mysqli_free_result($res);
		return $ret;
	}

	if (true !== ($tmp = mysqlnd_qc_set_storage_handler("apc")))
		printf("[001] APC handler rejected, got %s\n", var_export($tmp, true));

	require("table.inc");
	$fields_find = array(
		"cache_hits" 		=> "int",
		"valid_until"		=> "int",
		"min_run_time"		=> "float",
		"max_run_time"		=> "float",
		"avg_run_time"		=> "float",
		"min_store_time"	=> "float",
		"max_store_time"	=> "float",
		"avg_store_time"	=> "float",
	);

	$end = count($fields_find) + 100 + 1;
	for ($i = 100; $i < $end; $i++) {
		$query = sprintf("INSERT INTO test(id) VALUES (%d)", $i);
		if (!mysqli_query($link, $query))
			printf("[002] [%d] [%s]\n", mysqli_errno($link), mysqli_error($link));
	}

	$i = 100;
	$offset = 3;
	foreach ($fields_find as $field => $type) {
		$query = sprintf("%sSELECT id FROM test WHERE id = %d", $QC_SQL_HINT, ++$i);
		$row = run_query($offset++, $link, $query);
		if (empty($row)) {
			printf("[%03d] %s has returned no data\n", $offset++, $query);
			continue;
		}
		if (!mysqli_query($link, sprintf("DELETE FROM test WHERE id = %d", $i))) {
			printf("[%03d] [%d] %s\n", $offset++, mysqli_errno($link), mysqli_error($link));
			continue;
		}
		$tmp = find_cache_entry($query);
		list($key, $data) = each($tmp);
		if (!$key) {
			printf("[%03d] Cannot find query in APC user list\n", $offset++);
			continue;
		}
		unset($data[$field]);
 		apc_store($key, $data, 2);
		$info = mysqlnd_qc_get_cache_info();
		apc_clear_cache("user");
	}

	$end = count($fields_find) + 200 + 1;
	for ($i = 200; $i < $end; $i++) {
		$query = sprintf("INSERT INTO test(id) VALUES (%d)", $i);
		if (!mysqli_query($link, $query))
			printf("[%03d] [%d] [%s]\n", $offset++, mysqli_errno($link), mysqli_error($link));
	}

	$i = 200;
	foreach ($fields_find as $field => $type) {
		$query = sprintf("%sSELECT id FROM test WHERE id = %d", $QC_SQL_HINT, ++$i);
		$row = run_query($offset++, $link, $query);
		if (empty($row)) {
			printf("[%03d] %s has returned no data\n", $offset++, $query);
			continue;
		}
		if (!mysqli_query($link, sprintf("DELETE FROM test WHERE id = %d", $i))) {
			printf("[%03d] [%d] %s\n", $offset++, mysqli_errno($link), mysqli_error($link));
			continue;
		}
		$tmp = find_cache_entry($query);
		list($key, $data) = each($tmp);
		if (!$key) {
			printf("[%03d] Cannot find query in APC user list\n", $offset++);
			continue;
		}
		$data[$field] = "a";
 		apc_store($key, $data, 5);
		$row = run_query($offset++, $link, $query);
		apc_clear_cache("user");
	}

	$end = count($fields_find) + 300 + 1;
	for ($i = 300; $i < $end; $i++) {
		$query = sprintf("INSERT INTO test(id) VALUES (%d)", $i);
		if (!mysqli_query($link, $query))
			printf("[%03d] [%d] [%s]\n", $offset++, mysqli_errno($link), mysqli_error($link));
	}

	$i = 300;
	foreach ($fields_find as $field => $type) {
		$query = sprintf("%sSELECT id FROM test WHERE id = %d", $QC_SQL_HINT, ++$i);
		$row = run_query($offset++, $link, $query);
		if (empty($row)) {
			printf("[%03d] %s has returned no data\n", $offset++, $query);
			continue;
		}
		if (!mysqli_query($link, sprintf("DELETE FROM test WHERE id = %d", $i))) {
			printf("[%03d] [%d] %s\n", $offset++, mysqli_errno($link), mysqli_error($link));
			continue;
		}
		$tmp = find_cache_entry($query);
		list($key, $data) = each($tmp);
		if (!$key) {
			printf("[%03d] Cannot find query in APC user list\n", $offset++);
			continue;
		}
		unset($data[$field]);
 		apc_store($key, $data, 5);
		$row = run_query($offset++, $link, $query);
		apc_clear_cache("user");
	}

	$end = count($fields_find) + 400 + 1;
	for ($i = 400; $i < $end; $i++) {
		$query = sprintf("INSERT INTO test(id) VALUES (%d)", $i);
		if (!mysqli_query($link, $query))
			printf("[%03d] [%d] [%s]\n", $offset++, mysqli_errno($link), mysqli_error($link));
	}

	$i = 400;
	foreach ($fields_find as $field => $type) {
		$query = sprintf("%sSELECT id FROM test WHERE id = %d", $QC_SQL_HINT, ++$i);
		$row = run_query($offset++, $link, $query);
		if (empty($row)) {
			printf("[%03d] %s has returned no data\n", $offset++, $query);
			continue;
		}
		if (!mysqli_query($link, sprintf("DELETE FROM test WHERE id = %d", $i))) {
			printf("[%03d] [%d] %s\n", $offset++, mysqli_errno($link), mysqli_error($link));
			continue;
		}
		$tmp = find_cache_entry($query);
		list($key, $data) = each($tmp);
		if (!$key) {
			printf("[%03d] Cannot find query in APC user list\n", $offset++);
			continue;
		}
		$data[$field] = "wrong type";
 		apc_store($key, $data, 2);
		$info = mysqlnd_qc_get_cache_info();
		apc_clear_cache("user");
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: cache_hits not found) in %s on line %d

Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: min_run_time not found) in %s on line %d

Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: max_run_time not found) in %s on line %d

Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: avg_run_time not found) in %s on line %d

Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: min_store_time not found) in %s on line %d

Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: max_store_time not found) in %s on line %d

Warning: mysqlnd_qc_get_cache_info(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: avg_store_time not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type cache_hits) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type valid_until) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type min_run_time) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type max_run_time) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type avg_run_time) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type min_store_time) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type max_store_time) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type avg_store_time) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: cache_hits not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: valid_until not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: min_run_time not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: max_run_time not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: avg_run_time not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: min_store_time not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: max_store_time not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: avg_store_time not found) in %s on line %d
done!