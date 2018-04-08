--TEST--
Custom handler not derived from internal classes
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
--SKIPIF--
<?php
require_once('skipif.inc');
if (!extension_loaded('pcntl')){
	die('skip pcntl extension not available');
}
?>
--FILE--
<?php
	require("handler.inc");
	require("table.inc");
	require_once("user_handler_helper.inc");

	function fork_and_run($offset, $code) {
		global $link;

		$pid = pcntl_fork();
		if ($pid == -1) {
			die(sprintf("[%03d] Could not fork", $offset));
		} else if ($pid) {
			// we are the parent
			pcntl_wait($status);
			if (pcntl_wifexited($status))
				return pcntl_wexitstatus($status);
			return 0;
		} else {
			var_dump(eval($code));
			$class = "my_custom_handler" . $offset;
			$obj = new $class();
			mysqlnd_qc_set_storage_handler($obj);
			exit(0);
		}
	}

	$code = "
	class my_custom_handler1 implements mysqlnd_qc_handler {
	}
	";

	$tmp = fork_and_run(1, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[001] Failure\n");


	$code = "
	class my_custom_handler2 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
	}
	";
	$tmp = fork_and_run(2, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[002] Failure\n");

	$code = "
	class my_custom_handler3 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
	}
	";
	$tmp = fork_and_run(3, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[003] Failure\n");

	$code = "
	class my_custom_handler4 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
	}
	";
	$tmp =  fork_and_run(4, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[004] Failure\n");

	$code = "
	class my_custom_handler5 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
	}
	";
	$tmp = fork_and_run(5, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[005] Failure\n");

	$code = "
	class my_custom_handler6 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
		public function update_cache_stats() {}
	}
	";
	$tmp = fork_and_run(6, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[006] Failure\n");

	$code = "
	class my_custom_handler7 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
		public function update_cache_stats() {}
		public function get_stats() {}
	}
	";
	$tmp = fork_and_run(7, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[007] Failure\n");

	$code = "
	class my_custom_handler8 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
		public function update_cache_stats() {}
		public function get_stats() {}
		public function clear_cache() {}
	}
	";
	$tmp = fork_and_run(8, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[008] Failure\n");

	$code = "
	class my_custom_handler9 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
		public function update_cache_stats() {}
		public function get_stats() {}
		public function clear_cache() {}
		public function init() {}
	}
	";
	$tmp = fork_and_run(9, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[009] Failure\n");

	printf("missing: find_in_cache\n");
	$code = "
	class my_custom_handler10 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
		public function update_cache_stats() {}
		public function get_stats() {}
		public function clear_cache() {}
		public function init() { return true; }
		public function shutdown() { return true; }
	}
	";
	$tmp = fork_and_run(10, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[010] Failure\n");

	printf("missing: none\n");
	$code = "
	class my_custom_handler11 implements mysqlnd_qc_handler {
		public function get_hash_key() {}
		public function return_to_cache() {}
		public function add_to_cache() {}
		public function is_select() {}
		public function update_cache_stats() {}
		public function get_stats() {}
		public function clear_cache() {}
		public function init() { return true; }
		public function shutdown() { return true; }
		public function find_in_cache() {}
	}
	";
	$tmp = fork_and_run(11, $code);
	if (0 != $tmp && 255 != $tmp)
		printf("[0011] Failure\n");

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
Fatal error: Class my_custom_handler1 %s

Fatal error: Class my_custom_handler2 %s

Fatal error: Class my_custom_handler3 %s

Fatal error: Class my_custom_handler4 %s

Fatal error: Class my_custom_handler5 %s

Fatal error: Class my_custom_handler6 %s

Fatal error: Class my_custom_handler7 %s

Fatal error: Class my_custom_handler8 %s

Fatal error: Class my_custom_handler9 %s
missing: find_in_cache

Fatal error: Class my_custom_handler10 %s
missing: none
NULL
done!