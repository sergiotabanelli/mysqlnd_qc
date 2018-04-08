--TEST--
mysqlnd_qc_get_cache_info(), default handler
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--INI--
mysqlnd_qc.ttl=99
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
mysqlnd_qc.apc_prefix="mysqlnd_qc_apc_phpt"
--FILE--
<?php
	function diff($old, $new, $depth = 0) {
		if (is_array($old)) {
			foreach ($old as $k => $v) {
				if (!isset($new[$k])) {
					printf("%s missing %s\n", str_repeat('.', $depth), $k);
					continue;
				}
				if (is_array($v)) {
					diff($v, $new[$k], $depth + 1);
				} else {
					if ($v != $new[$k]) {
						printf("%s changed %s %s -> %s\n", str_repeat('.', $depth), $k, $v, $new[$k]);
					}
				}
				unset($new[$k]);
			}
			if (!empty($new)) {
				foreach ($new as $k => $v)
					printf("%s new %s\n", str_repeat('.', $depth), $k);
			}
		} else {
			if ($old != $new) {
				printf("%s changed %s -> %s\n", str_repeat('.', $depth), $old, $new);
			}
		}
	}

	require_once("handler.inc");
	if (!mysqlnd_qc_set_storage_handler("default")) {
		printf("[001] Failed to switch to default handler\n");
	}

	$before = mysqlnd_qc_get_cache_info();

	require("table.inc");
	$sql = sprintf("%s/*%s=%d*/%s", $QC_SQL_HINT, $QC_SQL_HINT_TTL, 3, "SELECT id, label FROM test WHERE id = 1");
	if (!$res = mysqli_query($link, $sql)) {
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	var_dump($row);

	$after = mysqlnd_qc_get_cache_info();
	var_dump($after);
	diff($before, $after);
	$before = $after;

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!$res = mysqli_query($link, $sql)) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);
	var_dump($row);

	/* Note: some handlers will report num_entries and stats others will not! */
	$after = mysqlnd_qc_get_cache_info();
	diff($before, $after);
	$before = $after;

	mysqli_close($link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
array(2) {
  ["id"]=>
  string(1) "1"
  ["label"]=>
  string(1) "a"
}
array(4) {
  ["num_entries"]=>
  int(1)
  ["handler"]=>
  string(7) "default"
  ["handler_version"]=>
  string(%d) "%s"
  ["data"]=>
  array(1) {
    ["%A
%s
%s
%s
%s"]=>
    array(2) {
      ["statistics"]=>
      array(12) {
        ["rows"]=>
        int(1)
        ["stored_size"]=>
        int(%d)
        ["cache_hits"]=>
        int(0)
        ["run_time"]=>
        int(%d)
        ["store_time"]=>
        int(%d)
        ["min_run_time"]=>
        int(%d)
        ["max_run_time"]=>
        int(%d)
        ["min_store_time"]=>
        int(%d)
        ["max_store_time"]=>
        int(%d)
        ["avg_run_time"]=>
        int(%d)
        ["avg_store_time"]=>
        int(%d)
        ["valid_until"]=>
        int(%d)
      }
      ["metadata"]=>
      array(2) {
        [0]=>
        array(%d) {
          ["name"]=>
          string(2) "id"
          ["orig_name"]=>
          string(2) "id"
          ["table"]=>
          string(4) "test"
          ["orig_table"]=>
          string(4) "test"
          ["db"]=>
          string(%d) "%s"
          ["max_length"]=>
          int(%d)
          ["length"]=>
          int(%d)
          ["type"]=>
          int(%f)
        }
        [1]=>
        array(8) {
          ["name"]=>
          string(5) "label"
          ["orig_name"]=>
          string(5) "label"
          ["table"]=>
          string(4) "test"
          ["orig_table"]=>
          string(4) "test"
          ["db"]=>
          string(%d) "%s"
          ["max_length"]=>
          int(%d)
          ["length"]=>
          int(%d)
          ["type"]=>
          int(%d)
        }
      }
    }
  }
}
 changed num_entries 0 -> 1
. new %s
%s
%s
%s
%s
array(2) {
  ["id"]=>
  string(1) "1"
  ["label"]=>
  string(1) "a"
}
... changed cache_hits 0 -> 1
... changed min_run_time 0 -> %d
... changed max_run_time 0 -> %d
... changed min_store_time 0 -> %d
... changed max_store_time 0 -> %d
... changed avg_run_time 0 -> %d
... changed avg_store_time 0 -> %d
done!