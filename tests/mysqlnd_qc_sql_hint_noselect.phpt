--TEST--
SQL Hint but no SELECT
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
	require_once("connect.inc");

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	if (!mysqli_query($link, $QC_SQL_HINT))
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!mysqli_query($link, $QC_SQL_HINT . "SET @myvar = 1"))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	if (!@mysqli_query($link, $QC_SQL_HINT . ""))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	mysqli_close($link);
	print "done!";
?>
--EXPECTF--
done!