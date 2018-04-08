--TEST--
Slam defense (needs pcntl and posix)
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.slam_defense=1
mysqlnd_qc.slam_defense_ttl=30
mysqlnd_qc.cache_no_table=1
mysqlnd_qc.ttl=1
--SKIPIF--
<?php
require_once('skipif.inc');

if (!function_exists('pcntl_fork'))
	die("skip Process Control Functions not available");

if (!function_exists('posix_getpid'))
	die("skip POSIX functions not available");

if (("userp" !== $env_handler) && ("apc" != $env_handler))
	die("skip Slam defense is not available with handler " . $env_handler);
?>
--FILE--
<?php
	$verbose = 0;
	require("handler.inc");
	require("table.inc");

	$script_start = microtime(true);

	function get_link() {
		global $host, $user, $passwd, $db, $port, $socket, $script_start;
		if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
			printf("[001] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n", $host, $user, $db, $port, $socket);
		}
		return $link;
	}

	function send_message($sender, $recipient, $message, $data) {
		global $verbose, $script_start;

		if ($verbose)
			printf("%-5.2f %-20s: (sender = %s, recipient = %s, message = %s, data = %s)\n",
				microtime(true) - $script_start, "send_message", $sender, $recipient, $message, $data);

		$link = get_link();
		$sql = sprintf("INSERT INTO messages(sender, recipient, command, data) VALUES ('%s', '%s', '%s', '%s')",
			$sender,
			$recipient,
			mysqli_real_escape_string($link, $message),
			mysqli_real_escape_string($link, $data));

		if (!($ret = mysqli_query($link, $sql)))
			printf("send_message - [%d] %s\n", mysqli_errno($link), mysqli_error($link));

		return $ret;
	}

	function child_to_parent($message, $data) {
		return send_message("child", "parent", $message, $data);
	}

	function parent_to_child($message, $data) {
		return send_message("parent", "child", $message, $data);
	}

	function pull_message($for, $block = false) {
		global $verbose, $sleep, $script_start;
		if ($verbose)
			printf("%-5.2f %-20s: (for = %s, block = %s)\n", microtime(true) - $script_start, "pull_message", $for, $block);

		$link = get_link();
		$sql = sprintf("SELECT id, command, data FROM messages WHERE recipient = '%s' AND processed = 'no' ORDER BY id ASC  LIMIT 1", $for);

		$loops = 0;
		$ret = array("message" => NULL, "data" => NULL);
		$start = microtime(true);
		do {
			$loops++;
			if ($verbose)
				printf("%-5.2f %-20s:\tloop = %d\n", microtime(true) - $script_start, "pull_message", $loops);
			if (!$block && ((microtime(true) - $start) > ($sleep + 1))) {
				printf("[002] Stopping to avoid endless loop\n");
				break;
			}
			if (!($res = mysqli_query($link, $sql))) {
				printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
				break;
			}
			$row = mysqli_fetch_assoc($res);
			mysqli_free_result($res);

			if (!empty($row)) {
				if (!mysqli_query($link, sprintf("UPDATE messages SET processed = 'yes' WHERE id = %d", $row['id']))) {
					printf("[004] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
				} else {
					$ret['message'] = $row['command'];
					$ret['data'] = $row['data'];
				}
				break;
			}
			usleep(300000);
		} while (true);

		if ($verbose)
			printf("%-5.2f %-20s:\tmessage = %s, data = %s\n", microtime(true) - $script_start, "pull_message", $ret['message'], $ret['data']);
		return $ret;
	}

	if (!mysqli_query($link, "DROP TABLE IF EXISTS messages") ||
		!mysqli_query($link, sprintf("
			CREATE TABLE messages(
				id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
				sender ENUM('parent', 'child') NOT NULL,
				recipient ENUM('parent', 'child') NOT NULL,
				command VARCHAR(127),
				data VARCHAR(127),
				processed ENUM('yes', 'no') NOT NULL DEFAULT 'no'
			) ENGINE = %s", $engine))) {
		printf("[005] [%d] %s\n", mysqli_errno($link), mysqli_error($link));
	}

	/* fill cache */
	$start = microtime(true);
	printf("%-5.2f %-20s\n", microtime(true) - $script_start, "start");

	$ttl = ini_get('mysqlnd_qc.ttl');
	$sleep = $ttl + 1;
	$select = sprintf("%sSELECT id, label, SLEEP(%d) AS _sleep FROM test WHERE id = 1", $QC_SQL_HINT, $sleep);

	if (!$res = mysqli_query($link, $select))
		printf("[006] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	$row = mysqli_fetch_assoc($res);
	mysqli_free_result($res);

	printf("%-5.2f %-20s\n", microtime(true) - $script_start, "cache entry generated");

	$pid = pcntl_fork();
	switch ($pid) {
		case -1:
			printf("[007] Cannot fork child");
			break;

		case 0:
			/* child */
			// printf("child: %d", posix_getpid());
			$child_link = get_link();
			child_to_parent("client will sleep until cache expires", microtime(true));
			sleep($ttl + 1);
			/* initial select has expired - we will get a cache miss and slam_defense shall hook in*/
			$last_run = microtime(true);
			child_to_parent("client wakes up to cause cache_miss for slam_defense", $last_run);
			printf("%-5.2f %-20s: mysqli_query for stale refresh\n", microtime(true) - $script_start, "child");
			if (!$child_res = mysqli_query($child_link, $select)) {
				printf("[008] [%d] %s\n", mysqli_errno($child_link), mysqli_error($child_link));
				exit(1);
			}
			/* query should have run several seconds */
			$rown = mysqli_fetch_assoc($child_res);
			mysqli_free_result($child_res);
			$duration = microtime(true) - $last_run;
			if ($duration < $ttl) {
				child_to_parent("no cache miss", $duration);
			} else if ($row != $rown) {
				child_to_parent("wrong results", serialize(array_diff($row, $rown)));
			} else if ($row == $rown) {
				child_to_parent("done", microtime(true));
			}
			exit(0);
			break;

		default:
			/* parent */
			$parent_link = get_link();

			define("PARENT_PARENT_START", 0);
			define("PARENT_CHILD_START", 1);
			define("PARENT_CHILD_END", 2);
			define("PARENT_CHECK_FINAL_STATS", 3);

			$parent_status = PARENT_PARENT_START;
			$status = NULL;
			$child_start = 0;
			$parent_row = $parent_rown = array();
			do {
				$wait_id = pcntl_waitpid($pid, $status, WNOHANG);
				switch ($parent_status) {
					case PARENT_PARENT_START:
						$message = pull_message("parent");
						$child_start = $message['data'];
						$parent_status = PARENT_CHILD_START;
						printf("%-5.2f %-20s: %s = %s\n", microtime(true) - $script_start, "child to parent", $message['message'], $message['data']);

						/* client will go to sleep, we should be able to create a cache hit */
						$now = microtime(true);
						if (!$res = mysqli_query($parent_link, $select))
							printf("[011] [%d] %s\n", mysqli_errno($parent_link), mysqli_error($parent_link));
						$parent_row = mysqli_fetch_assoc($res);
						mysqli_free_result($res);

						$runtime = microtime(true) - $now;
						printf("%-5.2f %-20s: parent cache_hit runtime = %.2fs\n", microtime(true) - $script_start, "parent", $runtime);
						break;

					case PARENT_CHILD_START:
						$message = pull_message("parent");
						usleep(10000);
						$child_start = $message['data'];
						$parent_status = PARENT_CHILD_END;
						printf("%-5.2f %-20s: %s = %s\n", microtime(true) - $script_start, "child to parent", $message['message'], $message['data']);
						/*
						Child has slept for TTL+1 seconds and is now refreshing the cache.
						We should get stale data.
						*/
						printf("%-5.2f %-20s: mysqli_query for stale hit\n", microtime(true) - $script_start, "parent");
						$now = microtime(true);
						if (!$res = mysqli_query($parent_link, $select))
							printf("[009] [%d] %s\n", mysqli_errno($parent_link), mysqli_error($parent_link));
						$parent_rown = mysqli_fetch_assoc($res);
						mysqli_free_result($res);

						$runtime = microtime(true) - $now;
						printf("%-5.2f %-20s: parent slam_hit, runtime = %.2fs\n", microtime(true) - $script_start, "parent", $runtime);
						if ($runtime >= $sleep) {
							printf("[010] Query seems not cached, runtime too long.\n");
							break 2;
						}

						if ($parent_row != $parent_rown) {
							printf("[15] Parent slam_defense hit seems to have returned borked data.\n");
							var_dump($parent_row);
							var_dump($parent_rown);
						}
						break;

					case PARENT_CHILD_END:
						$parent_status = PARENT_CHECK_FINAL_STATS;
						$now = microtime(true);
						if (!$res = mysqli_query($parent_link, $select))
							printf("[016] [%d] %s\n", mysqli_errno($parent_link), mysqli_error($parent_link));
						$parent_rown = mysqli_fetch_assoc($res);
						mysqli_free_result($res);

						$runtime = microtime(true) - $now;
						printf("%-5.2f %-20s: parent slam_hit, runtime = %.2fs\n", microtime(true) - $script_start, "parent", $runtime);
						if ($runtime >= $sleep) {
							printf("[017] Query seems not cached, runtime too short.\n");
							break 2;
						}

						$message = pull_message("parent");
						printf("%-5.2f %-20s: %s = %s\n", microtime(true) - $script_start, "child to parent", $message['message'], $message['data']);
						break;

					case PARENT_CHECK_FINAL_STATS:
						/* Finally we need to check if the updated cache entry has a life time of ttl */
						printf("%-5.2f %-20s: parent sleeps to check for cache_miss\n", microtime(true) - $script_start, "parent");

						sleep($ttl + 1);
						// expecting a cache miss (and not a cache slam hit because of slam_defense_ttl!)
						$now = microtime(true);
						if (!$res = mysqli_query($parent_link, $select))
							printf("[016] [%d] %s\n", mysqli_errno($parent_link), mysqli_error($parent_link));
						$parent_rown = mysqli_fetch_assoc($res);
						mysqli_free_result($res);

						$runtime = microtime(true) - $now;
						printf("%-5.2f %-20s: parent cache_miss, runtime = %.2fs\n", microtime(true) - $script_start, "parent", $runtime);
						if ($runtime < $sleep) {
							printf("[018] Query seems cached, runtime too short.\n");
							break 2;
						}

						switch ($env_handler) {
							case "apc":
								$stats = mysqlnd_qc_get_core_stats();
								// TODO - stats look borked
								// 	var_dump($stats);
								//	var_dump($env_handler);
								break;
						}
						/* yes, break 2 to exit */

						break 2;

					default:
						printf("[011] Invalid status, test must be buggy\n");
						break;
				}
			} while (true);

			$status = NULL;
			$wait_id = pcntl_waitpid($pid, $status);
			if (pcntl_wifexited($status) && (0 != ($tmp = pcntl_wexitstatus($status)))) {
				printf("[012] Exit code: %s\n", (pcntl_wifexited($status)) ? pcntl_wexitstatus($status) : 'n/a');
				printf("[013] Signal: %s\n", (pcntl_wifsignaled($status)) ? pcntl_wtermsig($status) : 'n/a');
				printf("[014] Stopped: %d\n", (pcntl_wifstopped($status)) ? pcntl_wstopsig($status) : 'n/a');
			}
			break;
	}

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
%f  star%s
%f  cache entry generated
%f  child to parent     : client will sleep until cache expires = %f
%f  parent              : parent cache_hit runtime = %d.%d%ds
%f  child               : mysqli_query for stale refresh
%f  child to parent     : client wakes up to cause cache_miss for slam_defense = %f
%f  parent              : mysqli_query for stale hit
%f  parent              : parent slam_hit, runtime = %d.%d%ds
%f  parent              : parent slam_hit, runtime = %d.%d%ds
%f  child to parent     : done = %f
%f  parent              : parent sleeps to check for cache_miss
%f parent              : parent cache_miss, runtime = %fs
done!