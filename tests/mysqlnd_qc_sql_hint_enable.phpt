--TEST--
SQL Hint to enable/disable caching
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
--FILE--
<?php
 	require_once("handler.inc");
	require("table.inc");

	function delete_and_fetch($offset, $link, $select, $delete) {

		if (!($res = mysqli_query($link, $select))) {
			printf("[%03d + 01] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
		}
		$row = mysqli_fetch_assoc($res);
		if (count($row) == 0) {
			printf("[%03d + 02 ] No data, test will fail\n", $offset);
			return array(NULL, NULL);
		}
		mysqli_free_result($res);

		if (!mysqli_query($link, $delete)) {
			printf("[%03d + 03] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return array(NULL, NULL);
		}

		if (!($res = mysqli_query($link, $select))) {
			printf("[%03d + 04] [%d] %s\n", $offset, mysqli_errno($link), mysqli_error($link));
			return array(NULL, NULL);
		}

		$rown = mysqli_fetch_assoc($res);
		mysqli_free_result($res);

		$ret = array($row, $rown);
		return $ret;
	}

	/* case insensitive */
	$on = sprintf("/*%s=oN*/", $QC_SQL_HINT_ENABLE);
	$off = sprintf("/*%s=oFf*/", $QC_SQL_HINT_ENABLE);

	$ret = delete_and_fetch(1, $link, $on  . "SELECT id FROM test WHERE id = 1", "DELETE FROM test WHERE id = 1");
	if (($ret[0] != $ret[1]) || is_null($ret[0])) {
		printf("[002] Enabling switch has failed, dumping data.\n");
		var_dump($ret[0]);
		var_dump($ret[1]);
	}

	$ret = delete_and_fetch(3, $link, $off  . "SELECT id FROM test WHERE id = 2", "DELETE FROM test WHERE id = 2");
	if (($ret[0] == $ret[1]) || is_null($ret[0])) {
		printf("[004] Disable switch has failed, dumping data.\n");
			var_dump($ret[0]);
		var_dump($ret[1]);
	}

	/* second SQL hint overrules first one */
	$ret = delete_and_fetch(5, $link, $off . $on . "SELECT id FROM test WHERE id = 3", "DELETE FROM test WHERE id = 3");
	if (($ret[0] != $ret[1]) || is_null($ret[0])) {
		printf("[006] Last SQL hint has not overruled previous ones, dumping data.\n");
			var_dump($ret[0]);
		var_dump($ret[1]);
	}

	/* invalid hint  -> use global = don't cache*/
	$invalid = sprintf("/*%s=0*/", $QC_SQL_HINT_ENABLE);
	$ret = delete_and_fetch(7, $link, $invalid . "SELECT id FROM test WHERE id = 4", "DELETE FROM test WHERE id = 4");
	if (($ret[0] == $ret[1]) || is_null($ret[0])) {
		printf("[008] Invalid SQL hint has caused caching , dumping data.\n");
		var_dump($ret[0]);
		var_dump($ret[1]);
	}

	/* SQL hint overrules ini setting */
	if (0 != ($tmp = ini_set("mysqlnd_qc.cache_by_default", 1))) {
		printf("[009] Cache by default had been enabled? Test should fail. Got: %s\n", var_export($tmp, true));
	}
	if (1 != ($tmp = ini_get("mysqlnd_qc.cache_by_default"))) {
		printf("[010] Cannot enable cache by default. Got: %s\n", var_export($tmp, true));
	}

	$ret = delete_and_fetch(11, $link, $off  . "SELECT id FROM test WHERE id = 5", "DELETE FROM test WHERE id = 5");
	if (($ret[0] == $ret[1]) || is_null($ret[0])) {
		printf("[012] Disable switch has failed to overrule cache_by_default, dumping data.\n");
		var_dump($ret[0]);
		var_dump($ret[1]);
	}

	mysqli_close($link);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
done!