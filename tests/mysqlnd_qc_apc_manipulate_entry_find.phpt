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
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require("table.inc");

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

	$fields_find = array(
		"cache_hits" 		=> "int",
		"valid_until"		=> "int",
	);

	$i = 0;
	$offset = 2;
	foreach ($fields_find as $field => $type) {

		$query = sprintf("%sSELECT id FROM test WHERE id = %d", $QC_SQL_HINT, ++$i);

		$row = run_query($offset++, $link, $query);
		if (empty($row)) {
			printf("[%03d] %s has returned no data\n", $offset++, $query);
			continue;
		}
		if (!mysqli_query($link, $sql = sprintf("DELETE FROM test WHERE id = %d", $i))) {
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
		$rown = run_query($offset++, $link, $query);
		if (!empty($rown)) {
			printf("[%03d] Unexpected result for query '%s'\n", $offset++, $query);
			var_dump($row);
			var_dump($rown);
			continue;
		}
	}

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
		$data[$field] = $data[$field] . " ";
 		apc_store($key, $data, 2);
		$rown = run_query($offset++, $link, $query);
		if (!empty($rown)) {
			printf("[%03d] Unexpected result for query '%s'\n", $offset++, $query);
			var_dump($row);
			var_dump($rown);
			continue;
		}
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: cache_hits not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: valid_until not found) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type cache_hits) in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: unexpected type valid_until) in %s on line %d
done!
