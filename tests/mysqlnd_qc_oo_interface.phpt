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
require_once("handler.inc");
if ("userp" == $env_handler)
	die("skip Not available with current handler");
?>
--FILE--
<?php
	require("handler.inc");
	require("table.inc");
	require_once("user_handler_helper.inc");

	abstract class my_abstract_custom_handler implements mysqlnd_qc_handler {
	}

	class my_custom_handler implements mysqlnd_qc_handler {

		public function get_hash_key() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("get_hash", func_get_args());
		}

		public function return_to_cache() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("return_to_cache", func_get_args());
		}

		public function add_to_cache() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("add", func_get_args());
		}

		public function is_select() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("is_select", func_get_args());
		}

		public function find_in_cache() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("find", func_get_args());
		}

		public function update_cache_stats() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("update_stats", func_get_args());
		}

		public function get_stats() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("get_stats", func_get_args());
		}

		public function clear_cache() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return call_user_func_array("clear_cache", func_get_args());
		}

		public function init() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return true;
		}

		public function shutdown() {
			printf("%s:%s(%s)\n", __CLASS__, __METHOD__, trim(var_export(func_get_args(), true)));
			return true;
		}
	}

	$obj = new my_custom_handler();
	mysqlnd_qc_set_is_select(array($obj, "is_select"));
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[010] Usage of %s failed\n", get_class($obj));
	}
	test_query(11, $link);
	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
my_custom_handler:my_custom_handler::init(array (
))
my_custom_handler:my_custom_handler::is_select(array (
  0 => '%s',
))
my_custom_handler:my_custom_handler::get_hash_key(array (
  0 => %s,
  1 => %s,
  2 => %s,
  3 => %s,
  4 => %s,
  5 => '/*%s*/SELECT * FROM test WHERE id = 1/* unique_id=%d */ ',
  6 => true,
))
my_custom_handler:my_custom_handler::find_in_cache(array (
  0 => '%s',
))
my_custom_handler:my_custom_handler::add_to_cache(array (
  0 => '%s',
  1 => '%s',
  2 => %d,
  3 => %d,
  4 => %d,
  5 => %d,
))
my_custom_handler:my_custom_handler::is_select(array (
  0 => 'DELETE FROM test WHERE id = 1',
))
my_custom_handler:my_custom_handler::is_select(array (
  0 => '/*%s*/SELECT * FROM test WHERE id = 1/* unique_id=%d */ ',
))
my_custom_handler:my_custom_handler::get_hash_key(array (
  0 => %s,
  1 => %s,
  2 => %s
  3 => %s,
  4 => %s,
  5 => '/*%s*/SELECT * FROM test WHERE id = 1/* unique_id=%d */ ',
  6 => true,
))
my_custom_handler:my_custom_handler::find_in_cache(array (
  0 => '%s',
))
my_custom_handler:my_custom_handler::return_to_cache(array (
  0 => '%s',
))
my_custom_handler:my_custom_handler::update_cache_stats(array (
  0 => '%s',
  1 => %d,
  2 => %d,
))
my_custom_handler:my_custom_handler::is_select(array (
  0 => '%s',
))
done!