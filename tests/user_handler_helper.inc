<?php
/* A couple of plain vanilly user handler callback functions */
define("MYSQLND_QC_UH_MODE_PERSISTENT", 0);
define("MYSQLND_QC_UH_MODE_NON_PERSISTENT", 1);

uh_set_mode(MYSQLND_QC_UH_MODE_NON_PERSISTENT);

function uh_set_mode($cache_mode) {
	global $__cache_mode;

	switch ($cache_mode) {
		case MYSQLND_QC_UH_MODE_PERSISTENT:
			$__cache_mode = $cache_mode;
			require_once("connect.inc");
			date_default_timezone_set("Europe/Berlin");
			uh_p_init();
			break;
		case MYSQLND_QC_UH_MODE_NON_PERSISTENT:
			$__cache_mode = $cache_mode;
			break;
		default:
			die(sprintf("Unknown user handler mode: '%s'", $__cache_mode));
			break;
	}
}

function uh_p_hidden_query($sql) {
	global $__cache_p_no_select, $verbose;
	$__cache_p_no_select = true;

	$sql = sprintf("/*%s*/%s\n", MYSQLND_QC_DISABLE_SWITCH, $sql);
	$link = uh_p_get_connection();
	return mysqli_query($link, $sql);
}

function uh_p_init() {
	static $called = false;
	if ($called)
		return true;

	$link = uh_p_get_connection();
	/* ignoring test engine... */
	if (!uh_p_hidden_query( "DROP TABLE IF EXISTS uh_p_cache_entries") ||
		! uh_p_hidden_query( "CREATE TABLE uh_p_cache_entries(
			cache_key VARCHAR(32) NOT NULL PRIMARY KEY,
			data blob NOT NULL,
			valid_until DATETIME NOT NULL,
			run_time FLOAT UNSIGNED,
			store_time FLOAT UNSIGNED,
			row_count INT UNSIGNED,
			in_refresh ENUM('yes', 'no') DEFAULT 'no' NOT NULL,
			in_refresh_until DATETIME DEFAULT NULL,
			hits INT UNSIGNED,
			slam_hits INT UNSIGNED,
			INDEX idx_key(cache_key, valid_until))")) {
		printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));
		return false;
	}

	return true;
}

function uh_p_get_connection($new = true) {
	global $host, $user, $passwd, $db, $port, $socket;
	static $connection = NULL;

	if (!$new && !is_null($connection))
		return $connection;

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		die(sprintf("Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket));
	}

	if (!$new)
		$connection = $link;

	return $link;
}

function clear_cache_void() {
	global $__cache;
}

function clear_cache_np() {
	global $__cache;
	$__cache = array();
	return true;
}

function clear_cache_p() {
	$link = uh_p_get_connection();
	return mysqli_query($link, "DELETE FROM uh_p_cache_entries");
}

function clear_cache() {
	global $__cache_mode;
	switch ($__cache_mode) {
		case MYSQLND_QC_UH_MODE_PERSISTENT:
			return clear_cache_p();
			break;
		case MYSQLND_QC_UH_MODE_NON_PERSISTENT:
			return clear_cache_np();
			break;
		default:
			die(sprintf("Unknown user handler mode: '%s'", $__cache_mode));
			break;
	}
}

function get_hash($host_info, $port, $charset, $user, $db, $query, $server_id) {
	global $__cache, $verbose;
	if ($verbose)
		printf("get_hash('%s', '%s', '%s', '%s', '%s', '%s', '%s')\n", $host_info, $port, $charset, $user, $db, $query, $server_id);

	if ($server_id) {
		$ret = md5(sprintf("%s%s", $server_id, $query));
	} else {
		$ret = md5(sprintf("%s%s%s%s%s%s", $host_info, $port, $charset, $user, $db, $query));
	}
	if ($verbose)
		printf("\tret = '%s'\n", $ret);

	return $ret;
}

function get_hash_void($host_info, $port, $charset, $user, $db, $query, $server_id) {
	global $verbose;
	if ($verbose)
		printf("get_hash_void() proxy\n");
	get_hash($host_info, $port, $charset, $user, $db, $query, $server_id);
}

function find_np($key) {
	global $__cache, $verbose;
	if ($verbose)
		printf("find('%s')\n", $key);

	if (isset($__cache[$key])) {
		$now = time();
		if ($__cache[$key]['valid_until'] < $now) {
			if (ini_get("mysqlnd_qc.slam_defense")) {
				if (!$__cache[$key]['in_refresh']) {
					$__cache[$key]['in_refresh'] = true;
					$ret = NULL;
					if ($verbose > 1)
						printf("\tin_refresh set\n");
				} else {
					$__cache[$key]['slam_hits']++;
					$ret = $__cache[$key]["data"];
					if ($verbose > 1)
						printf("\tslam_hit\n");
				}
			} else {
				$ret = NULL;
				unset($__cache[$key]);
			}
		} else {
			if ($verbose > 1)
				printf("\tsill valid - now = %d, valid_until = %d\n", $now, $__cache[$key]['valid_until']);
			$ret = $__cache[$key]["data"];
			$__cache[$key]["hits"]++;
		}
	} else {
		$ret = NULL;
	}

	if ($verbose)
		printf("\tret = %s[%d]\n", gettype($ret), strlen($ret));

	return $ret;
}

function find_p($key) {
	global $verbose;
	if ($verbose)
		printf("find_p('%s')\n", $key);

	/* we don't care about concurrent accesses */
	$link = uh_p_get_connection();
	if (!$link)
		die(sprintf("Can't connect [%d] %s\n", mysqli_connect_errno(), mysqli_connect_error()));

	$sql = sprintf("SELECT * FROM uh_p_cache_entries WHERE cache_key = '%s'", mysqli_real_escape_string($link, $key));

	if ($res = uh_p_hidden_query( $sql)) {
		$row = mysqli_fetch_assoc($res);
		if (false != $row) {
			mysqli_free_result($res);

			if ($row['in_refresh'] == 'yes') {
				if ($verbose > 1)
					printf("\tin_refresh\n");

				if (strtotime($row['in_refresh_until']) < time()) {
					// in_refresh has expired
					$ret = NULL;
					if ($verbose > 1)
						printf("\tin_refresh_until has expired, cache_miss");

					$sql = sprintf("DELETE FROM uh_p_cache_entries WHERE cache_key = '%s'", mysqli_real_escape_string($link, $key));
						uh_p_hidden_query( $sql);

				} else {
					$ret = $row['data'];

					if ($verbose > 1)
						printf("\tslam_ hit\n");

					$sql = sprintf("UPDATE uh_p_cache_entries SET slam_hits = %d cache_key = '%s'", $row['slam_hits'] + 1, mysqli_real_escape_string($link, $key));
					uh_p_hidden_query( $sql);
				}

			} else {
				$now = time();
				$valid_until = strtotime($row['valid_until']);
				if ($valid_until < $now) {

					if (ini_get("mysqlnd_qc.slam_defense")) {
						// expired but slam defense

						$ret = NULL;
						if ($verbose > 1)
							printf("\tslam defense\n");

						$sql = sprintf("
						UPDATE
							uh_p_cache_entries
						SET
							in_refresh = 'yes',
							in_refresh_until = '%s'
						 WHERE
							cache_key = '%s'",
							mysqli_real_escape_string($link, date('Y-m-d H:i:s', time() + ini_get("mysqlnd_qc.slam_defense_ttl"))),
							mysqli_real_escape_string($link, $key)
						);
					 	uh_p_hidden_query( $sql);

					} else {

						// expired
						$ret = NULL;

						$sql = sprintf("DELETE FROM uh_p_cache_entries WHERE cache_key = '%s'", mysqli_real_escape_string($link, $key));
						uh_p_hidden_query( $sql);
					}

				} else {
					// valid

					$ret = $row['data'];

					$sql = sprintf("UPDATE uh_p_cache_entries SET hits = %d cache_key = '%s'", $row['hits'] + 1, mysqli_real_escape_string($link, $key));
					 uh_p_hidden_query( $sql);
				}
			}

		} else {
			$ret = NULL;
		}
	} else {
		$ret = NULL;
	}

	if ($verbose)
		printf("\tret = %s[%d]\n", gettype($ret), strlen($ret));

	return $ret;
}

function find_void($key) {
	global $verbose;
	if ($verbose)
		printf("find_void() proxy\n");
}

function find($key) {
	global $__cache_mode;
	switch ($__cache_mode) {
		case MYSQLND_QC_UH_MODE_PERSISTENT:
			return find_p($key);
			break;
		case MYSQLND_QC_UH_MODE_NON_PERSISTENT:
			return find_np($key);
			break;
		default:
			die(sprintf("Unknown user handler mode: '%s'", $__cache_mode));
			break;
	}
}

function add_np($key, $data, $ttl, $run_time, $store_time, $row_count) {
	global $__cache, $verbose;
	if ($verbose)
		printf("add('%s', %s[%d], %d, %.4f, %.4f, %d)\n", $key, gettype($data), strlen($data), $ttl, $run_time, $store_time, $row_count);

	$__cache[$key] = array(
		"data" 			=> $data,
		"valid_until" 	=> time() + $ttl,
		"hits" 			=> 0,
		"run_time" 		=> array(),
		"store_time" 	=> array(),
		"in_refresh"	=> false,
		"slam_hits"		=> 0,
	);
	return true;
}

function add_p($key, $data, $ttl, $run_time, $store_time, $row_count) {
	global $verbose;
	if ($verbose)
		printf("add('%s', %s[%d], %d, %.4f, %.4f, %d)\n", $key, gettype($data), strlen($data), $ttl, $run_time, $store_time, $row_count);

	$link = uh_p_get_connection();
	$sql = sprintf("REPLACE INTO uh_p_cache_entries (cache_key, data, valid_until, run_time, store_time, row_count, in_refresh, in_refresh_until, hits, slam_hits) VALUES ('%s', '%s', '%s', %f, %f, %d, 'no', NULL, 0, 0)",
		mysqli_real_escape_string($link, $key),
		mysqli_real_escape_string($link, $data),
		date("Y-m-d H:i:s", time() + $ttl),
		$run_time,
		$store_time,
		$row_count);

	if (!$ret = uh_p_hidden_query( $sql))
		printf("[%d] %s\n", mysqli_errno($link), mysqli_error($link));

	return $ret;
}

function add_void($key, $data, $ttl, $run_time, $store_time, $row_count) {
	global $verbose;
	if ($verbose)
		printf("add_void()\n");
}

function add($key, $data, $ttl, $run_time, $store_time, $row_count) {
	global $__cache_mode;
	switch ($__cache_mode) {
		case MYSQLND_QC_UH_MODE_PERSISTENT:
			return add_p($key, $data, $ttl, $run_time, $store_time, $row_count);
			break;
		case MYSQLND_QC_UH_MODE_NON_PERSISTENT:
			return add_np($key, $data, $ttl, $run_time, $store_time, $row_count);
			break;
		default:
			die(sprintf("Unknown user handler mode: '%s'", $__cache_mode));
			break;
	}
}

function is_select_np($query) {
	global $QC_SQL_HINT, $QC_SQL_HINT_ENABLE, $QC_SQL_HINT_TTL, $QC_SQL_HINT_SERVER_ID, $verbose;

	if ($verbose)
		printf("is_select('%s')\n", $query);

	$ret = $cache_by_default = (ini_get("mysqlnd_qc.cache_by_default")) ? true : false;
	$ttl = (int)ini_get("mysqlnd_qc.ttl");
	$server_id = NULL;
	$token = tokenize_sql($query);

	if (empty($token)) {
		die(sprintf("EEEK, tokenizer is not good enough, %s @ %d", __FILE__, __LINE__));
	} else {

		$sql_hint_on = sprintf("/*%s=on*/", $QC_SQL_HINT_ENABLE);
		$sql_hint_off = sprintf("/*%s=off*/", $QC_SQL_HINT_ENABLE);
		$sql_hint_ttl_regexp = sprintf("@/\*%s=(.+)\*/@is", $QC_SQL_HINT_TTL);
		$sql_hint_server_id_regexp = sprintf("@/\*%s=(.+)\*/@is", $QC_SQL_HINT_SERVER_ID);
		foreach ($token as $k => $tok) {
			if ($tok['type'] != MYSQLND_QC_UH_COMMENT)
				break;

			if (($ret == false) && (0 == strcasecmp($tok['value'], $sql_hint_on)))
				$ret = true;

			if (($ret == true) && (0 == strcasecmp($tok['value'], $sql_hint_off)))
				$ret = false;

			if (preg_match($sql_hint_ttl_regexp, $tok['value'], $matches)) {
				$ttl = (int)trim($matches[1]);
			}

			if (preg_match($sql_hint_server_id_regexp, $tok['value'], $matches)) {
				$server_id = trim($matches[1]);
			}
		}

		$ret = $ret && ($tok['type'] == MYSQLND_QC_UH_SELECT);
	}
	// var_dump($token);
	if (true == $ret) {
		$ret = array('ttl' => (int)$ttl, 'server_id' => $server_id);
	} else {
		$ret = false;
	}
	$ret = (true === $ret) ? (int)$ttl : $ret;

	if ($verbose)
		printf("\tret = %s\n", var_export($ret, true));

	return $ret;
}

define("MYSQLND_QC_UH_COMMENT", 1);
define("MYSQLND_QC_UH_SELECT", 2);
define("MYSQLND_QC_UH_OTHER", 3);
define("MYSQLND_QC_UH_ST_OTHER", 4);
define("MYSQLND_QC_UH_ST_COMMENT", 5);
define("MYSQLND_QC_UH_ST_SELECT", 6);

/*
Very, very basic tokenizer - it is just about good enough for the tests.
*/
function tokenize_sql($query) {
	$len = strlen($query);

	$token = array();
	$buffer = "";
	$state = MYSQLND_QC_UH_ST_OTHER;
	for ($pos = 0; $pos < $len; $pos++) {
		$char = substr($query, $pos, 1);
		$buffer .= $char;
//		printf("%03d '%s' '%s'\n", $pos, $char, $buffer);

		switch ($char) {
			case "/":
				switch ($state) {
					case MYSQLND_QC_UH_ST_COMMENT:
						break;
					default:
						if (substr($query, $pos, 2) == "/*") {
							$state = MYSQLND_QC_UH_ST_COMMENT;
							$pos++;
							$buffer = "/*";
							continue;
						}
						break;
				}
				break;
			case "*":
				switch ($state) {
					case MYSQLND_QC_UH_ST_COMMENT:
						if (substr($query, $pos, 2) == "*/") {
							$state = MYSQLND_QC_UH_ST_OTHER;
							$pos++;
							$token[] = array("type" => MYSQLND_QC_UH_COMMENT, "value" => $buffer . "/");
							$buffer = "";
							continue;
						}
						break;
					default:
						break;
				}
				break;
			case "s":
			case "S":
				switch ($state) {
					case MYSQLND_QC_UH_ST_OTHER:
						if (strtolower(substr($query, $pos, 6)) == "select") {
							$state = MYSQLND_QC_UH_ST_SELECT;
							$token[] = array("type" => MYSQLND_QC_UH_SELECT, "value" => substr($query, $pos));
							/* exit */
							$pos = $len;
						}
						break;
					case MYSQLND_QC_UH_ST_COMMENT:
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
	if (empty($token))
		$token = array("type" => MYSQLND_QC_UH_OTHER, "value" => $query);

	return $token;
}

function is_select_void($query) {
	global $verbose;
	if ($verbose)
		printf("is_select_void() proxy\n");
	is_select($query);
}

function is_select_p($query) {
	global $__cache_p_no_select, $verbose;

	if ($__cache_p_no_select) {
		$__cache_p_no_select = $ret = false;

		if ($verbose > 2)
			printf("is_select('%s')\n", $query);
		if ($verbose > 2)
			printf("\tret = %s\n", var_export($ret, true));

		return $ret;
	}

	$old_verbose = (int)$verbose;
	$verbose = 0;
	$ret = is_select_np($query);
	if (($old_verbose > 0) && ($old_verbose <= 2) && ($ret !== false)) {
		printf("is_select('%s')\n", $query);
		printf("\tret = %s\n", var_export($ret, true));
	} else if ($old_verbose > 2) {
		printf("is_select('%s')\n", $query);
		printf("\tret = %s\n", var_export($ret, true));
	}
	$verbose = $old_verbose;

	return $ret;
}

function is_select($query) {
	global $__cache_mode;
	switch ($__cache_mode) {
		case MYSQLND_QC_UH_MODE_PERSISTENT:
			return is_select_p($query);
			break;
		case MYSQLND_QC_UH_MODE_NON_PERSISTENT:
			return is_select_np($query);
			break;
		default:
			die(sprintf("Unknown user handler mode: '%s'", $__cache_mode));
			break;
	}
}

function return_to_cache($key) {
	global $verbose;
	if ($verbose)
		printf("return_to_cache('%s')\n", $key);
}

function update_stats_np($key, $run_time, $store_time) {
	global $verbose, $__cache;;

	if ($verbose)
		printf("update_stats(%s)\n", var_export(func_get_args(), true));


	if (isset($__cache[$key])) {
		$__cache[$key]["run_time"][] = $run_time;
		$__cache[$key]["store_time"][] = $store_time;
	}
}

function update_stats_p($key, $run_time, $store_time) {
	global $verbose;

	if ($verbose)
		printf("update_stats(%s)\n", var_export(func_get_args(), true));

	$link = uh_p_get_connection();
	$sql = sprintf("SELECT * FROM uh_p_cache_entries WHERE cache_key = '%s'", mysqli_real_escape_string($link, $key));

	if ($res = uh_p_hidden_query( $sql)) {
		$row = mysqli_fetch_assoc($res);

		if ($row['hits'] > 0) {
			$avg_run_time = ((($row['hits'] - 1) * $row['run_time']) + $run_time) / $row['hits'];
			$avg_store_time = ((($row['hits'] - 1) * $row['store_time']) + $store_time) / $row['hits'];
		} else {
			$avg_run_time = $run_time;
			$avg_store_time = $store_time;
		}

		$sql = sprintf("UPDATE uh_p_cache_entries SET run_time = %f, store_time = %f WHERE cache_key = '%s'", $run_time, $store_time, mysqli_real_escape_string($link, $key));
		uh_p_hidden_query( $sql);
	}

}

function update_stats($key, $run_time, $store_time) {
	global $__cache_mode;
	switch ($__cache_mode) {
		case MYSQLND_QC_UH_MODE_PERSISTENT:
			return update_stats_p($key, $run_time, $store_time);
			break;
		case MYSQLND_QC_UH_MODE_NON_PERSISTENT:
			return update_stats_np($key, $run_time, $store_time);
			break;
		default:
			die(sprintf("Unknown user handler mode: '%s'", $__cache_mode));
			break;
	}
}


function dummy() {
	global $verbose;
	if ($verbose)
		printf("dummy(%s)\n", var_export(func_get_args(), true));
}

function get_stats($key = NULL) {
	global $__cache;
	global $verbose;

	if ($verbose)
		printf("get_stats(%s)\n", var_export(func_get_args(), true));

	$stats = array();
	if (is_null($key)) {
		if (!empty($__cache)) {
			foreach ($__cache as $key => $details) {
				$stats[$key] = array('run_time' => $details['run_time'], 'store_time' => $details['store_time']);
			}
		}
	} else {
		if (isset($__cache[$key]))
			$stats[$key] = array('run_time' => $__cache[$key]['run_time'], 'store_time' => $__cache[$key]['store_time']);
	}
	return $stats;
}

/* Utility functions */
function test_query($offset, $link) {
	global $verbose;

	$query = randsql("SELECT * FROM test WHERE id = 1");
	if ($verbose)
		printf("test_query = %s\n", $query);
	if (!$res = mysqli_query($link, $query)) {
		printf("[%03d + 01] [%d] %s (%s)\n", $offset, mysqli_errno($Link), mysqli_error($link), $query);
		return false;
	}
	$row = mysqli_fetch_assoc($res);
	if ($verbose)
		printf("\tcolumns = %d\n", count($row));
	mysqli_free_result($res);

	if (!mysqli_query($link, "DELETE FROM test WHERE id = 1")) {
		printf("[%03d + 02] [%d] %s\n", $offset, mysqli_errno($Link), mysqli_error($link));
		return false;
	}
	if ($verbose)
		printf("\tDELETE\n");

	if (!$res = mysqli_query($link, $query)) {
		printf("[%03d + 03] [%d] %s (%s)\n", $offset, mysqli_errno($Link), mysqli_error($link), $query);
		die("DELETE will cause failures");
	}
	if ($verbose)
		printf("\tSELECT again\n");

	$rown = mysqli_fetch_assoc($res);
	if ($verbose)
		printf("\tcolumns = %d\n", count($rown));

	mysqli_free_result($res);

	if ($row != $rown) {
		printf("[%03d + 04] Not cached (%s)\n", $offset, $query);
		var_dump($row);
		var_dump($rown);
		die("DELETE will cause failures");
	}

	$fields = $values = '';
	foreach ($row as $field => $value) {
		$fields .= sprintf("%s, ", $field);
		$values .= sprintf("'%s', ", $value);
	}
	$query = sprintf("INSERT INTO test(%s) VALUES (%s)", substr($fields, 0, -2), substr($values, 0, -2));
	if (!mysqli_query($link, $query)) {
		printf("[%03d + 05] [%d] %s (%s)\n", $offset, mysqli_errno($link), mysqli_error($link), $query);
		return false;
	}

	return true;
}
?>