--TEST--
Extending build-in storage class: constructor, destructor, init/shutdown order
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	require("handler.inc");
	require("table.inc");
	$verbose = false;
	require_once("user_handler_helper.inc");

	class my_default_handler12 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public function is_select() {
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			return call_user_func_array("update_stats", func_get_args());
		}
		public function get_stats() {
			return call_user_func_array("get_stats", func_get_args());
		}
		public function clear_cache() {
			return call_user_func_array("clear_cache", func_get_args());
		}
		public function init() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return true;
		}
		public function shutdown() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			var_dump($this);
			return true;
		}
		public function __destruct() {
			/* not sure if we can make it the last one called */
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
		}
	}

	$obj = new my_default_handler12();
	mysqlnd_qc_set_is_select(array($obj, "is_select"));
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[001] Usage of %s failed\n", get_class($obj));
	}
	test_query(2, $link);
	print "done!\n";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
my_default_handler12::__construct(0)
my_default_handler12:my_default_handler12::init(0)
done!
my_default_handler12:my_default_handler12::__destruct(0)