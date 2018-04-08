--TEST--
Many, somtimes bogus, SQL hints
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
	require_once("table.inc");

	if (!($res = mysqli_query($link, "/*how*//*do you like*//*qc caching?*/" . "SELECT id FROM test WHERE id = 1")))
		printf("[001] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1"))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!($res = mysqli_query($link, "/*how*//*do you like*//*qc caching?*/" . "SELECT id FROM test WHERE id = 1")))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (($row == $rown) || empty($row)) {
		printf("[004] Query got cached or table has not been set up properly, dumping data\n");
		var_dump($row);
		var_dump($rown);
	}

	if (!($res = mysqli_query($link, "/*how*//*do you like*//*qc=On*/" . "SELECT id FROM test WHERE id = 2")))
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 2"))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!($res = mysqli_query($link, "/*how*//*do you like*//*qc=On*/" . "SELECT id FROM test WHERE id = 2")))
		printf("[007] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (($row != $rown) || empty($row)) {
		printf("[008] Query got not cached or table has not been set up properly, dumping data\n");
		var_dump($row);
		var_dump($rown);
	}

	if (!($res = mysqli_query($link, "/*how*//*do you like*//*qc=On*//*not the last comment*/" . "SELECT id FROM test WHERE id = 3")))
		printf("[009] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 3"))
		printf("[010] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!($res = mysqli_query($link, "/*how*//*do you like*//*qc=On*//*not the last comment*/" . "SELECT id FROM test WHERE id = 3")))
		printf("[011] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$rown = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	if (($row != $rown) || empty($row)) {
		printf("[012] Query got not cached or table has not been set up properly, dumping data\n");
		var_dump($row);
		var_dump($rown);
	}

	mysqli_close($link);
	print "done!";
?>
--EXPECTF--
done!