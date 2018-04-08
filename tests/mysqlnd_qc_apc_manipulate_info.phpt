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
	if (true !== ($tmp = mysqlnd_qc_set_storage_handler("apc")))
		printf("[001] APC handler rejected, got %s\n", var_export($tmp, true));

	require("table.inc");
	if (!$res = mysqli_query($link, "SELECT id FROM test WHERE id = 1")) {
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row1 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1")) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row2 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row1 != $row2) {
		printf("[004] Result sets should not differ\n");
		var_dump($row1);
		var_dump($row2);
	}

	$protected = array('recorded_data' => true, 'cache_hits' => true, 'valid_until' => true, 'in_refresh_until' => true);
	$manipulate = array();

	$prefix = ini_get('mysqlnd_qc.apc_prefix');
	$prefix_len = strlen($prefix);

	$hits = 0;
	$user = apc_cache_info("user");
	foreach ($user['cache_list'] as $data) {
		if (substr($data['info'], 0, $prefix_len) == $prefix) {
			$cached_query = apc_fetch($data['info']);
			$org_cached_query = $cached_query;
			if (empty($cached_query))
				continue;

			if (empty($manipulate)) {
				foreach ($cached_query as $k => $v) {
					if (!isset($protected[$k]))
						$manipulate[$k] = $k;
				}
				if (empty($manipulate)) {
					printf("[005] Cache entry looks already borked - no columns found to manipulate\n!");
					var_dump($cached_query);
					break;
				}
			}

			$hits++;

			foreach ($manipulate as $field) {
				unset($cached_query[$field]);

				if (!apc_store($data['info'], $cached_query)) {
					printf("[006] Failed to manupulate field '%s'\n", $field);
					break 2;
				}

				ob_start();
				$info = mysqlnd_qc_get_cache_info();
				$warning = ob_get_contents();
				ob_end_clean();

				if ("" == $warning)
					printf("[007] Cache corruption of field '%s' not detected\n", $field);

				if ($info['data'] != array())
					printf("[008] Stats although cache corrupted because of '%s': %s\n", $field, var_export($info));

				$cached_query = $org_cached_query;
				if (!apc_store($data['info'], $cached_query)) {
					printf("[009] Failed to restore original cache data\n");
					break 2;
				}

			}

		}
	}

	if (1 !== $hits)
		printf("[010] Found %d cache entries, expecting 1\n", $hits);


	if (!mysqli_query($link, "DELETE FROM test WHERE id < 2"))
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1")) {
		printf("[012] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row3 = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if ($row2 !== $row3) {
		printf("[013] Result sets should not differ\n");
		var_dump($row2);
		var_dump($row3);
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!