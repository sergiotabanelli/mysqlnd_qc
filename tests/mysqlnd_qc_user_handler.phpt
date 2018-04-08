--TEST--
User handler - permutations
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
--FILE--
<?php
	require_once("handler.inc");

	function try_handler($offset, $link, $exp, $get, $find, $add, $is_select, $return_to_cache, $clear_cache, $update_stats, $get_stats, $is_cached) {
		global $QC_SQL_HINT;
		mysqlnd_qc_set_is_select($is_select);
		if ($exp !== ($tmp = (mysqlnd_qc_set_user_handlers($get, $find, $return_to_cache, $add, $update_stats, $get_stats, $clear_cache))))
			printf("[%03d + 1] Expecting %s got %s\n", $offset, var_export($exp, true), var_export($tmp, true));
		if ($tmp !== true)
			return;

		if (!mysqli_query($link, "DELETE FROM test WHERE id = 1") ||
			!mysqli_query($link, "INSERT INTO test(id, label) VALUES (1, 'a')"))
			printf("[%03d + 2] [%d] [%s]\n", $offset, mysqli_errno($link), mysqli_error($link));

		$query = $QC_SQL_HINT . "SELECT id, label FROM test WHERE id = 1";
		if (!$res = mysqli_query($link, $query))
			printf("[%03d + 3] [%d] [%s]\n", $offset, mysqli_errno($link), mysqli_error($link));

		$rows = array();
		while ($row = mysqli_fetch_assoc($res))
			$rows[] = $row;

		if (count($rows) != 1)
			printf("[%03d + 4] Expecting 1 row got %d row(s)\n", $offset, count($rows));

		if ($res && (mysqli_num_rows($res) != 1))
			printf("[%03d + 5] Expecting 1 row got %d row(s)\n", $offset, mysqli_num_rows($res));

		if ($res)
			mysqli_free_result($res);

		if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
			printf("[%03d + 6] [%d] [%s]\n", $offset, mysqli_errno($link), mysqli_error($link));

		if (!$res = mysqli_query($link, $query))
			printf("[%03d + 7] [%d] [%s]\n", $offset, mysqli_errno($link), mysqli_error($link));

		$rows2 = array();
		while ($row = mysqli_fetch_assoc($res))
			$rows2[] = $row;

		if (!$is_cached) {
			if (!empty($rows2)) {
				printf("[%03d + 8] Resultset has not been taken from cache or cache is borked\n", $offset);
				var_dump($rows2);
			}
		} else {

			if ($rows !== $rows2) {
				printf("[%03d + 9] Result sets differ, dumping data\n", $offset);
			} else {
				if (count($rows2) != 1)
					printf("[%03d + 10] Expecting 1 row got %d row(s)\n", $offset, count($rows2));

				if ($res && (mysqli_num_rows($res) != 1))
					printf("[%03d + 11] Expecting 1 row got %d row(s)\n", $offset, mysqli_num_rows($res));
			}
		}

		if ($res)
			mysqli_free_result($res);
	}
	require_once('user_handler_helper.inc');
	require('table.inc');


	$select_handler = array('is_select' => true, 'is_select_void' => false);
	$add_handler = array('add' => true, 'add_void' => false);
	$find_handler = array('find' => true, 'find_void' => false);
	$hash_handler = array('get_hash' => true, 'get_hash_void' => false);
	$to_cache_handler = array('return_to_cache' => true);
	$clear_cache_handler = array('clear_cache' => true, 'clear_cache_void' => true);
	$update_stats_handler = array('update_stats' => true);
	$get_stats_handler = array('get_stats' => true);
	$permutation = 1;

	foreach ($select_handler as $select => $is_cached_select) {
		foreach ($add_handler as $add => $is_cached_add) {
			foreach ($find_handler as $find => $is_cached_find) {
				foreach ($hash_handler as $hash => $is_cached_hash) {
					foreach ($to_cache_handler as $to_cache => $is_cached_to_cache) {
						foreach ($clear_cache_handler as $clear_cache => $is_cached_clear_cache) {
							foreach ($update_stats_handler as $stats => $is_cached_stats) {
								foreach ($get_stats_handler as $get_stats => $is_cached_get_stats) {
									$is_really_cached = $is_cached_select && $is_cached_add && $is_cached_find && $is_cached_hash && $is_cached_to_cache && $is_cached_clear_cache && $is_cached_stats && $is_cached_get_stats;
									printf("%02d %-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %d\n", $permutation++, $select, $add, $find, $hash, $to_cache, $clear_cache, $stats, $get_stats, $is_really_cached);
									$verbose = false;
									clear_cache();
									try_handler($permutation, $link, true, $hash, $find, $add, $select, $to_cache, 	$clear_cache, $stats, $get_stats, $is_really_cached);
									if ($is_cached_hash) {
										printf("%02d %-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %d\n", $permutation++, $select, $add, $find, $hash, $to_cache, $clear_cache, $stats, $get_stats, $is_really_cached);
										try_handler($permutation, $link, true, $hash, $find, $add, $select, $to_cache, $clear_cache, $stats, $get_stats, $is_really_cached);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
01 is_select       add             find            get_hash        return_to_cache clear_cache     update_stats    get_stats       1
02 is_select       add             find            get_hash        return_to_cache clear_cache     update_stats    get_stats       1
03 is_select       add             find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       1
04 is_select       add             find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       1
05 is_select       add             find            get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
[006 + 8] Resultset has not been taken from cache or cache is borked
array(1) {
  [0]=>
  array(2) {
    ["id"]=>
    string(1) "1"
    ["label"]=>
    string(1) "a"
  }
}
06 is_select       add             find            get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
[007 + 8] Resultset has not been taken from cache or cache is borked
array(1) {
  [0]=>
  array(2) {
    ["id"]=>
    string(1) "1"
    ["label"]=>
    string(1) "a"
  }
}
07 is_select       add             find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0
08 is_select       add             find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0
09 is_select       add             find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0
10 is_select       add             find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0
11 is_select       add             find_void       get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
12 is_select       add             find_void       get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
13 is_select       add_void        find            get_hash        return_to_cache clear_cache     update_stats    get_stats       0
14 is_select       add_void        find            get_hash        return_to_cache clear_cache     update_stats    get_stats       0
15 is_select       add_void        find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       0
16 is_select       add_void        find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       0
17 is_select       add_void        find            get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
18 is_select       add_void        find            get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
19 is_select       add_void        find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0
20 is_select       add_void        find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0
21 is_select       add_void        find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0
22 is_select       add_void        find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0
23 is_select       add_void        find_void       get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
24 is_select       add_void        find_void       get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Hash key is empty in %s on line %d
25 is_select_void  add             find            get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
26 is_select_void  add             find            get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
27 is_select_void  add             find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
28 is_select_void  add             find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
29 is_select_void  add             find            get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
30 is_select_void  add             find            get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
31 is_select_void  add             find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
32 is_select_void  add             find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
33 is_select_void  add             find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
34 is_select_void  add             find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
35 is_select_void  add             find_void       get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
36 is_select_void  add             find_void       get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
37 is_select_void  add_void        find            get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
38 is_select_void  add_void        find            get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
39 is_select_void  add_void        find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
40 is_select_void  add_void        find            get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
41 is_select_void  add_void        find            get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
42 is_select_void  add_void        find            get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
43 is_select_void  add_void        find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
44 is_select_void  add_void        find_void       get_hash        return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
45 is_select_void  add_void        find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
46 is_select_void  add_void        find_void       get_hash        return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
47 is_select_void  add_void        find_void       get_hash_void   return_to_cache clear_cache     update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
48 is_select_void  add_void        find_void       get_hash_void   return_to_cache clear_cache_void update_stats    get_stats       0

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d

Warning: mysqli_query(): (mysqlnd_qc) Return value must be boolean or an array in %s on line %d
done!