--TEST--
mysqlnd_qc_get_cache_info()
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');

if (("user" == $env_handler) || ("userp" != $env_handler))
	die("skip Not available with user handler " . $env_handler);
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.time_statistics=1;
--FILE--
<?php
	require_once("handler.inc");
	require("table.inc");

	$query = $QC_SQL_HINT  . "SELECT id FROM test AS longtablename WHERE id = 1";
	if (!$res = mysqli_query($link, $query)) {
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	$fields = mysqli_fetch_fields($res);
	mysqli_free_result($res);

	$info = mysqlnd_qc_get_cache_info();
	if (empty($info['data'])) {
		// Driver does not support stats - valid!
		die("done: - no stats");
	}

	$found = false;
	foreach ($info['data'] as $key => $details) {
		if (false == stristr($key, $query))
			continue;

		if (!isset($details['metadata']) || empty($details['metadata'])) {
			// NULL, empty array, whatever - driver does not support it
			die("done: no metadata");
		}
		$found = true;
		break;
	}

	if (!$found)
		printf("[002] Hash key logic must have changed, can't find query data\n");

	/*
		We got a handler that has meta in cache stats.
		Handlers which support it should have a common minimum set of fields.
	*/
	$num_fields = count($details['metadata']);
	if ($num_fields != count($fields))
		printf("[003] Expecting metadata for %d field[s], got only %d\n", count($fields), $num_fields);



	$cache_to_fields = array(
		'name'			=> array(true, 'name'),
		'orig_name' 	=> array(true, 'orgname'),
		'table'			=> array(true, 'table'),
		'orig_table'	=> array(true, 'orgtable'),
		'db'			=> array(false, $db),
		'max_length'	=> array(true, 'max_length'),
		'length'		=> array(true, 'length'),
		'type'			=> array(true, 'type'),
	);

	foreach ($details['metadata'] as $idx => $meta) {
		if (!isset($fields[$idx])) {
			printf("[004] Confusing indexing (%d) which is not in line with mysqli_fetch_fields()\n", $idx);
			continue;
		}

		foreach ($cache_to_fields as $cache_field => $field_storage) {
			if (!isset($meta[$cache_field])) {
				printf("[005] Common cache meta field '%s' not found\n", $cache_field);
				continue 2;
			}
			if ($field_storage[0]) {
				if ($fields[$idx]->$field_storage[1] != $meta[$cache_field]) {
					printf("[006] fetch_fields(%s) = '%s' and cache_meta(%s) = '%s' differ\n",
						$field_storage[1], $fields[$idx]->$field_storage[1], $cache_field, $meta[$cache_field]);
				}
			} else {
				if ($field_storage[1] != $meta[$cache_field]) {
					printf("[007] cache_meta(%s) = '%s' differs from '%s'\n",
						$cache_field, $meta[$cache_field], $field_storage[1]);
				}
			}
			unset($meta[$cache_field]);
		}
		if (!empty($meta)) {
			printf("[008] Meta not empty - does the cache_to_field mapping need an update?\n");
			var_dump($meta);
		}
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done%s