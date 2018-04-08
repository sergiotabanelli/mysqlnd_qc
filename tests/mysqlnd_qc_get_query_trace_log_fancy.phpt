--TEST--
mysqlnd_qc_get_[normalized]_query_trace_log()
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
mysqlnd_qc.time_statistics=1;
mysqlnd_qc.collect_statistics=1
mysqlnd_qc.collect_query_trace=1
mysqlnd_qc.collect_normalized_query_trace=1
mysqlnd_qc.query_trace_bt_depth=19
mysqlnd_qc.cache_no_table=1
--FILE--
<?php
	require_once("handler.inc");
	require_once("connect.inc");

	function analyze_bt($trace) {
		$max_depth = 0;
		foreach ($trace as $k => $bt)  {
			printf("%-20s: %s\n", "query", $bt['query']);
			$origin = preg_split("@#\d@isu", $bt['origin'], 0,  PREG_SPLIT_NO_EMPTY);
			if (count($origin) > $max_depth)
				$max_depth = count($origin);
			printf("%-20s: %s\n", "depth", count($origin));
			foreach ($origin as $k => $v) {
				if (preg_match("@([^(]+)\((\d+)\)\:(.*)@isu", $v, $matches)) {
					printf("%-20s: %d - %d - %s\n", "origin", $k, $matches[2], trim($matches[3]));
				} else {
					// main and stuff
					printf("%-20s: %s\n", "origin", $k, trim($v));
				}
			}
			printf("\n");
		}
		return $max_depth;
	}

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
	}

	$query = $QC_SQL_HINT . "SELECT id FROM test WHERE id = 1";
	if (!$res = mysqli_query($link, $query)) {
		printf("[002] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	mysqli_free_result($res);

	$query = $QC_SQL_HINT . "SELECT id FROM test WHERE id = 2";
	if (!$res = $link->query($query)) {
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	mysqli_free_result($res);

	$query = $QC_SQL_HINT . "SELECT id FROM test WHERE id = 3";
	$code = eval('$res = mysqli_query($link, $query);');
	if (!$res) {
		printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	mysqli_free_result($res);

	$query = $QC_SQL_HINT . "SELECT id FROM test WHERE id = 4";
	$code = assert('$res = mysqli_query($link, $query);');
	if (!$res) {
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	mysqli_free_result($res);


$code = '
class myclass {
	private static $me;
	private $link;
	private $query;

	private function __construct($link, $query) {
		$this->link = $link;
		$this->query = $query;
	}
	public function query($depth) {
		if ($depth < 10) {
			$depth++;
			return $this->query($depth);
		}
		return $this->link->query($this->query);
	}
	public static function init($link, $query) {
		self::$me = new myclass($link, $query);
		self::$me->query(0);
		return self::$me;
	}

}
$obj = myclass::init($link, $query);
$res = $obj->query(5);
';

	$query = $QC_SQL_HINT . "SELECT id FROM test WHERE id = 5";
	eval($code);
	if (!$res) {
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}
	mysqli_free_result($res);

	if (($max_depth = analyze_bt(mysqlnd_qc_get_query_trace_log())) > 19)
		printf("[007] Max depth must not be larger than 4 - found %d!\n", $max_depth);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
query               : /*%s*/SELECT id FROM test WHERE id = 1
depth               : 2
origin              : 0 - 31 - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1

query               : /*%s*/SELECT id FROM test WHERE id = 2
depth               : 2
origin              : 0 - %d - mysqli->query('/*%s*/SELECT...')
origin              : 1

query               : /*%s*/SELECT id FROM test WHERE id = 3
depth               : 3
origin              : 0 - 1 - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - eval()
origin              : 2

query               : /*%s*/SELECT id FROM test WHERE id = 4
depth               : 3
origin              : 0 - 1 - mysqli_query(Object(mysqli), '/*%s*/SELECT...')
origin              : 1 - %d - assert('$res = mysqli_q...')
origin              : 2

query               : /*%s*/SELECT id FROM test WHERE id = 5
depth               : 15
origin              : 0 - %d - mysqli->query('/*%s*/SELECT...')
origin              : 1 - %d - myclass->query(10)
origin              : 2 - %d - myclass->query(9)
origin              : 3 - %d - myclass->query(8)
origin              : 4 - %d - myclass->query(7)
origin              : 5 - %d - myclass->query(6)
origin              : 6 - %d - myclass->query(5)
origin              : 7 - %d - myclass->query(4)
origin              : 8 - %d - myclass->query(3)
origin              : 9 - %d - myclass->query(2)
origin              : 10 - %d - myclass->query(1)
origin              : 11 - %d - myclass->query(0)
origin              : 12 - %d - myclass::init(Object(mysqli), '/*%s*/SELECT...')
origin              : 13 - %d - eval()
origin              : %d

query               : /*%s*/SELECT id FROM test WHERE id = 5
depth               : 9
origin              : 0 - %d - mysqli->query('/*%s*/SELECT...')
origin              : 1 - %d - myclass->query(10)
origin              : 2 - %d - myclass->query(9)
origin              : 3 - %d - myclass->query(8)
origin              : 4 - %d - myclass->query(7)
origin              : 5 - %d - myclass->query(6)
origin              : 6 - %d - myclass->query(5)
origin              : 7 - %d - eval()
origin              : 8

done!