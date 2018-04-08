--TEST--
Fetch query results multiple times
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
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	require_once('handler.inc');
	require("table.inc");
	if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1")) {
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	for ($i = 0; $i < 2; $i++) {
		if (!$res = mysqli_query($link, $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1")) {
			printf("[003 + %02d] [%d] %s\n", $i, mysqli_errno($link), mysqli_error($link));
		}

		$row2 = mysqli_fetch_assoc($res);
		mysqli_free_result($res);

		if ($row2 !== $row) {
			printf("[004 + %02d] Data differs - dumping\n", $i);
			var_dump($row);
			var_dump($row2);
		}
	}

	mysqli_close($link);

	print "done!";
?>
--EXPECTF--
done!