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
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_phpt"
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
		}
		$query = preg_replace("/\s+/", "\\s*", preg_quote($query));

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

	if (true !== ($tmp = mysqlnd_qc_set_storage_handler("apc")))
		printf("[001] APC handler rejected, got %s\n", var_export($tmp, true));


	$query1 = "SELECT id FROM test WHERE id = 1";
	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query1))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row1 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	$tmp = find_cache_entry($query1);
	list($key1, $cached_query1) = each($tmp);
	if (false == $cached_query1)
		printf("[003] Cannot find query in cache\n");

	$query2 = "SELECT id FROM test WHERE id = 2";
	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query2))
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row2 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	$tmp = find_cache_entry($query2);
	list($key2, $cached_query2) = each($tmp);
	if (false == $cached_query2)
		printf("[005] Cannot find query in cache\n");

	/* flip and manipulate query results */
	if (!apc_store($key1, $cached_query2) || !apc_store($key2, $cached_query1))
		printf("[006] Cannot flip results\n");

	if (!mysqli_query($link, "DELETE FROM test WHERE id < 2"))
		printf("[009] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query1))
		printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row1n = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row1n != $row2) {
		printf("[011] Flipping results failed - which is a good thing in general...\n");
		var_dump($row1n);
		var_dump($row2);
	}

	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query2))
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row2n = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row2n != $row1) {
		printf("[013] Flipping results failed - which is a good thing in general...\n");
		var_dump($row2n);
		var_dump($row1);
	}

	unset($cached_query1['recorded_data']);
	if (!apc_store($key1, $cached_query1))
		printf("[014] Cannot restore results\n");

	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query1))
		printf("[015] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	var_dump(mysqli_fetch_assoc($res));
	mysqli_free_result($res);

	unset($cached_query2['valid_until']);
	if (!apc_store($key2, $cached_query2))
		printf("[016] Cannot restore results\n");

	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query2))
		printf("[017] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	var_dump(mysqli_fetch_assoc($res));
	mysqli_free_result($res);

	$cached_query2['valid_until'] = time();
	$cached_query2['recorded_data'] = "I love the Query Cache!";
	if (!apc_store($key2, $cached_query2))
		printf("[018] Cannot restore results\n");

	if (!$res = mysqli_query($link, $QC_SQL_HINT . $query2))
		printf("[019] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	var_dump(mysqli_fetch_assoc($res));
	mysqli_free_result($res);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: recorded_data not found) in %s on line %d
NULL

Warning: mysqli_query(): (mysqlnd_qc) Cache entry '%a' seems corrupted (details: valid_until not found) in %s on line %d
array(1) {
  ["id"]=>
  string(1) "2"
}

Warning: Packets out of order. Expected %d received %d. Packet size=%s in %s on line %d

Warning: mysqli_query(): MySQL server has gone away in %s on line %d

Warning: mysqli_query(): Error reading result set's header in %s on line %d
[019] [2006] MySQL server has gone away

Warning: mysqli_fetch_assoc() expects parameter 1 to be mysqli_result, boolean given in %s on line %d
NULL

Warning: mysqli_free_result() expects parameter 1 to be mysqli_result, boolean given in %s on line %d
done!