--TEST--
Interface of the class mysqlnd_qc_handler_default
--SKIPIF--
<?php
require_once('skipif.inc');
?>
--FILE--
<?php
	$handler = new mysqlnd_qc_handler_default();

	printf("Class:\n");
	var_dump(get_class($handler));

	printf("Class exists:\n");
	var_dump(class_exists("mysqlnd_qc_handler_default"));

	printf("Parent class:\n");
	var_dump(get_parent_class($handler));

	printf("\nMethods:\n");
	$methods = get_class_methods($handler);

	$expected_methods = array(
		"get_hash_key"			=> true,
		"find_in_cache"			=> true,
		"return_to_cache"		=> true,
		"add_to_cache"			=> true,
		"update_cache_stats"	=> true,
		"get_stats"				=> true,
		"clear_cache"			=> true,
		"init"					=> true,
		"shutdown"				=> true,
	);

	foreach ($expected_methods as $method => $v) {
		if (!method_exists($handler, $method)) {
			printf("method_exists(%s) returns false\n", $method);
		}
	}

	foreach ($methods as $k => $method) {
		if (isset($expected_methods[$method])) {
			unset($expected_methods[$method]);
			unset($methods[$k]);
		}
	}
	if (!empty($expected_methods)) {
		printf("Dumping list of missing methods.\n");
		var_dump($expected_methods);
	}
	if (!empty($methods)) {
		printf("Dumping list of unexpected methods.\n");
		var_dump($methods);
	}
	if (empty($expected_methods) && empty($methods))
		printf("ok\n");

	printf("\nis_*:\n");
	printf("is_a(mysqlnd_qc_handler): %s\n", (is_a($handler, "mysqlnd_qc_handler")) ? 'yes' : 'no');
	printf("is_a(mysqlnd_qc_handler_default): %s\n", (is_a($handler, "mysqlnd_qc_handler_default")) ? 'yes' : 'no');
	printf("is_subclass_of(mysqlnd_qc_handler): %s\n", (is_subclass_of($handler, "mysqlnd_qc_handler")) ? 'yes' : 'no');
	printf("is_subclass_of(mysqlnd_qc_handler_default): %s\n", (is_subclass_of($handler, "mysqlnd_qc_handler_default")) ? 'yes' : 'no');

	$expected_variables = array("entries" => true);
	foreach ($expected_variables as $var => $v)
		if (!property_exists($handler, $var)) {
			printf("property_exists(%s) returns false\n", $var);
		}

	printf("\nClass variables:\n");
	$variables = array_keys(get_class_vars(get_class($handler)));
	sort($variables);
	foreach ($variables as $k => $var)
		printf("%s\n", $var);

	printf("\nObject variables:\n");
	$variables = array_keys(get_object_vars($handler));
	foreach ($variables as $k => $var)
		printf("%s\n", $var);

	printf("\nProperties:\n");
	printf("handler->entries = %s\n", var_export($handler->entries, true));
	printf("handler->unknown = %s\n", var_export($handler->unknown, true));

	print "done!";
?>
--EXPECTF--
Class:
string(26) "mysqlnd_qc_handler_default"
Class exists:
bool(true)
Parent class:
string(18) "mysqlnd_qc_handler"

Methods:
ok

is_*:
is_a(mysqlnd_qc_handler): yes
is_a(mysqlnd_qc_handler_default): yes
is_subclass_of(mysqlnd_qc_handler): yes
is_subclass_of(mysqlnd_qc_handler_default): no

Class variables:
entries

Object variables:
entries

Properties:
handler->entries = NULL

Notice: Undefined property: %s
handler->unknown = NULL
done!
