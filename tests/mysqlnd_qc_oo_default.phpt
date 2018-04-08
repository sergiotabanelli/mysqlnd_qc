--TEST--
Extending build-in storage class
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

	class my_default_handler extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
	}

	$obj = new mysqlnd_qc_handler_default();
	mysqlnd_qc_set_is_select("mysqlnd_qc_default_query_is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[001] Usage of mysqlnd_qc_handler_default failed\n");
	}
	test_query(20, $link);

	mysqlnd_qc_clear_cache();

	$obj = new my_default_handler();
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[030] Usage of %s failed\n", get_class($obj));
	}
	test_query(31, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler2 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
	}
	$obj = new my_default_handler2();
	mysqlnd_qc_set_is_select("my_default_handler2::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[032] Usage of %s failed\n", get_class($obj));
	}
	test_query(33, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler3 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
	}
	$obj = new my_default_handler3();
	mysqlnd_qc_set_is_select("my_default_handler3::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[034] Usage of %s failed\n", get_class($obj));
	}
	test_query(35, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler4 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
	}
	$obj = new my_default_handler4();
	mysqlnd_qc_set_is_select("my_default_handler4::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[036] Usage of %s failed\n", get_class($obj));
	}
	test_query(37, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler6 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
	}
	$obj = new my_default_handler6();
	mysqlnd_qc_set_is_select("my_default_handler6::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[038] Usage of %s failed\n", get_class($obj));
	}
	test_query(39, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler7 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("update_stats", func_get_args());
		}
	}
	$obj = new my_default_handler7();
	mysqlnd_qc_set_is_select("my_default_handler7::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[040] Usage of %s failed\n", get_class($obj));
	}
	test_query(41, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler8 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("update_stats", func_get_args());
		}
		public function get_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_stats", func_get_args());
		}
	}
	$obj = new my_default_handler8();
	mysqlnd_qc_set_is_select("my_default_handler8::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[042] Usage of %s failed\n", get_class($obj));
	}
	test_query(43, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler9 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("update_stats", func_get_args());
		}
		public function get_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_stats", func_get_args());
		}
		public function clear_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("clear_cache", func_get_args());
		}
	}
	$obj = new my_default_handler9();
	mysqlnd_qc_set_is_select("my_default_handler9::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[044] Usage of %s failed\n", get_class($obj));
	}
	test_query(45, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler10 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("update_stats", func_get_args());
		}
		public function get_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_stats", func_get_args());
		}
		public function clear_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("clear_cache", func_get_args());
		}
		public function init() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return true;
		}
	}
	$obj = new my_default_handler10();
	mysqlnd_qc_set_is_select("my_default_handler10::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[046] Usage of %s failed\n", get_class($obj));
	}
	test_query(47, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler11 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("update_stats", func_get_args());
		}
		public function get_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_stats", func_get_args());
		}
		public function clear_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("clear_cache", func_get_args());
		}
		public function init() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return true;
		}
		public function shutdown() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return true;
		}
	}
	$obj = new my_default_handler11();
	printf("... 11 again\n");
	mysqlnd_qc_set_is_select("my_default_handler11::is_select");
	$obj = new my_default_handler11();
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[050] Usage of %s failed\n", get_class($obj));
	}
	test_query(51, $link);
	mysqlnd_qc_clear_cache();

	class my_default_handler12 extends mysqlnd_qc_handler_default {
		public function __construct() {
			printf("%s::__construct(%d)\n", get_class($this), func_num_args());
		}
		public static function is_select() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("is_select", func_get_args());
		}
		public function get_hash_key() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_hash", func_get_args());
		}
		public function return_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("return_to_cache", func_get_args());
		}
		public function add_to_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("add", func_get_args());
		}
		public function find_in_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("find", func_get_args());
		}
		public function update_cache_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("update_stats", func_get_args());
		}
		public function get_stats() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("get_stats", func_get_args());
		}
		public function clear_cache() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return call_user_func_array("clear_cache", func_get_args());
		}
		public function init() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return true;
		}
		public function shutdown() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
			return true;
		}
		public function __destruct() {
			printf("%s:%s(%d)\n", __CLASS__, __METHOD__, func_num_args());
		}
	}

	$obj = new my_default_handler12();
	mysqlnd_qc_set_is_select("my_default_handler12::is_select");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[052] Usage of %s failed\n", get_class($obj));
	}
	test_query(53, $link);

	mysqlnd_qc_clear_cache();

	if (array_search('mysqlnd_qc_handler_sqlite', get_declared_classes()) === true) {
		echo "handler_sqlite\n";
		$obj = new mysqlnd_qc_handler_sqlite();
		mysqlnd_qc_set_is_select("mysqlnd_qc_default_query_is_select");
		if (!mysqlnd_qc_set_storage_handler($obj)) {
			printf("[053] Usage of %s failed\n", get_class($obj));
		}
		test_query(54, $link);
	}

	printf("handler_default\n");
	$obj = new mysqlnd_qc_handler_default();
	$abcxyz = $obj->entries;
	$abcxyz = $obj->get_stats();
	$obj = NULL; // force destruct
	mysqlnd_qc_set_is_select("mysqlnd_qc_default_query_is_select");

	$obj = new mysqlnd_qc_handler_default();
	printf("should call mshutdown and destruct of the old one\n");
	if (!mysqlnd_qc_set_storage_handler($obj)) {
		printf("[054] Usage of %s failed\n", get_class($obj));
	} else {
		printf("Handler mysqlnd_qc_handler_default set\n");
	}
	mysqlnd_qc_get_cache_info();
	test_query(55, $link);

	print "done!";
?>
--CLEAN--
<php require_once("clean_table.inc"); ?>
--EXPECTF--
my_default_handler::__construct(0)
my_default_handler2::__construct(0)
my_default_handler2:my_default_handler2::is_select(1)
my_default_handler2:my_default_handler2::is_select(1)
my_default_handler2:my_default_handler2::is_select(1)
my_default_handler2:my_default_handler2::is_select(1)
my_default_handler3::__construct(0)
my_default_handler3:my_default_handler3::is_select(1)
my_default_handler3:my_default_handler3::get_hash_key(7)
my_default_handler3:my_default_handler3::is_select(1)
my_default_handler3:my_default_handler3::is_select(1)
my_default_handler3:my_default_handler3::get_hash_key(7)
my_default_handler3:my_default_handler3::is_select(1)
my_default_handler4::__construct(0)
my_default_handler4:my_default_handler4::is_select(1)
my_default_handler4:my_default_handler4::get_hash_key(7)
my_default_handler4:my_default_handler4::is_select(1)
my_default_handler4:my_default_handler4::is_select(1)
my_default_handler4:my_default_handler4::get_hash_key(7)
my_default_handler4:my_default_handler4::return_to_cache(1)
my_default_handler4:my_default_handler4::is_select(1)
my_default_handler6::__construct(0)
my_default_handler6:my_default_handler6::is_select(1)
my_default_handler6:my_default_handler6::get_hash_key(7)
my_default_handler6:my_default_handler6::find_in_cache(1)
my_default_handler6:my_default_handler6::add_to_cache(6)
my_default_handler6:my_default_handler6::is_select(1)
my_default_handler6:my_default_handler6::is_select(1)
my_default_handler6:my_default_handler6::get_hash_key(7)
my_default_handler6:my_default_handler6::find_in_cache(1)
my_default_handler6:my_default_handler6::return_to_cache(1)
my_default_handler6:my_default_handler6::is_select(1)
my_default_handler7::__construct(0)
my_default_handler7:my_default_handler7::is_select(1)
my_default_handler7:my_default_handler7::get_hash_key(7)
my_default_handler7:my_default_handler7::find_in_cache(1)
my_default_handler7:my_default_handler7::add_to_cache(6)
my_default_handler7:my_default_handler7::is_select(1)
my_default_handler7:my_default_handler7::is_select(1)
my_default_handler7:my_default_handler7::get_hash_key(7)
my_default_handler7:my_default_handler7::find_in_cache(1)
my_default_handler7:my_default_handler7::return_to_cache(1)
my_default_handler7:my_default_handler7::update_cache_stats(3)
my_default_handler7:my_default_handler7::is_select(1)
my_default_handler8::__construct(0)
my_default_handler8:my_default_handler8::is_select(1)
my_default_handler8:my_default_handler8::get_hash_key(7)
my_default_handler8:my_default_handler8::find_in_cache(1)
my_default_handler8:my_default_handler8::add_to_cache(6)
my_default_handler8:my_default_handler8::is_select(1)
my_default_handler8:my_default_handler8::is_select(1)
my_default_handler8:my_default_handler8::get_hash_key(7)
my_default_handler8:my_default_handler8::find_in_cache(1)
my_default_handler8:my_default_handler8::return_to_cache(1)
my_default_handler8:my_default_handler8::update_cache_stats(3)
my_default_handler8:my_default_handler8::is_select(1)
my_default_handler9::__construct(0)
my_default_handler9:my_default_handler9::is_select(1)
my_default_handler9:my_default_handler9::get_hash_key(7)
my_default_handler9:my_default_handler9::find_in_cache(1)
my_default_handler9:my_default_handler9::add_to_cache(6)
my_default_handler9:my_default_handler9::is_select(1)
my_default_handler9:my_default_handler9::is_select(1)
my_default_handler9:my_default_handler9::get_hash_key(7)
my_default_handler9:my_default_handler9::find_in_cache(1)
my_default_handler9:my_default_handler9::return_to_cache(1)
my_default_handler9:my_default_handler9::update_cache_stats(3)
my_default_handler9:my_default_handler9::is_select(1)
my_default_handler9:my_default_handler9::clear_cache(0)
my_default_handler10::__construct(0)
my_default_handler10:my_default_handler10::init(0)
my_default_handler10:my_default_handler10::is_select(1)
my_default_handler10:my_default_handler10::get_hash_key(7)
my_default_handler10:my_default_handler10::find_in_cache(1)
my_default_handler10:my_default_handler10::add_to_cache(6)
my_default_handler10:my_default_handler10::is_select(1)
my_default_handler10:my_default_handler10::is_select(1)
my_default_handler10:my_default_handler10::get_hash_key(7)
my_default_handler10:my_default_handler10::find_in_cache(1)
my_default_handler10:my_default_handler10::return_to_cache(1)
my_default_handler10:my_default_handler10::update_cache_stats(3)
my_default_handler10:my_default_handler10::is_select(1)
my_default_handler10:my_default_handler10::clear_cache(0)
my_default_handler11::__construct(0)
... 11 again
my_default_handler11::__construct(0)
my_default_handler11:my_default_handler11::init(0)
my_default_handler11:my_default_handler11::is_select(1)
my_default_handler11:my_default_handler11::get_hash_key(7)
my_default_handler11:my_default_handler11::find_in_cache(1)
my_default_handler11:my_default_handler11::add_to_cache(6)
my_default_handler11:my_default_handler11::is_select(1)
my_default_handler11:my_default_handler11::is_select(1)
my_default_handler11:my_default_handler11::get_hash_key(7)
my_default_handler11:my_default_handler11::find_in_cache(1)
my_default_handler11:my_default_handler11::return_to_cache(1)
my_default_handler11:my_default_handler11::update_cache_stats(3)
my_default_handler11:my_default_handler11::is_select(1)
my_default_handler11:my_default_handler11::clear_cache(0)
my_default_handler12::__construct(0)
my_default_handler11:my_default_handler11::shutdown(0)
my_default_handler12:my_default_handler12::init(0)
my_default_handler12:my_default_handler12::is_select(1)
my_default_handler12:my_default_handler12::get_hash_key(7)
my_default_handler12:my_default_handler12::find_in_cache(1)
my_default_handler12:my_default_handler12::add_to_cache(6)
my_default_handler12:my_default_handler12::is_select(1)
my_default_handler12:my_default_handler12::is_select(1)
my_default_handler12:my_default_handler12::get_hash_key(7)
my_default_handler12:my_default_handler12::find_in_cache(1)
my_default_handler12:my_default_handler12::return_to_cache(1)
my_default_handler12:my_default_handler12::update_cache_stats(3)
my_default_handler12:my_default_handler12::is_select(1)
my_default_handler12:my_default_handler12::clear_cache(0)
handler_default
should call mshutdown and destruct of the old one
my_default_handler12:my_default_handler12::shutdown(0)
my_default_handler12:my_default_handler12::__destruct(0)
Handler mysqlnd_qc_handler_default set
done!