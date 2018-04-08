--TEST--
mysqlnd_qc_get_available_handlers()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	$handler = mysqlnd_qc_get_available_handlers();

	if (NULL !== ($tmp = @mysqlnd_qc_get_available_handlers(1))) {
		printf("[001] Expecting NULL got %s\n", var_export($tmp, true));
	}

	$known_handler = array('default' => true, 'user' => true, 'apc' => false, 'memcache' => false);
	foreach ($handler as $name => $info) {
		$name = strtolower($name);
		if (!isset($info['version']) || !is_string($info['version']) || '' == $info['version'])
			printf("[002] %s seems to have wrong version string, got %s\n", $name, var_export($info, true));
		if (!isset($info['version_number']) || !is_int($info['version_number']) || $info['version_number'] < 100000)
			printf("[003] %s seems to have wrong version number, got %s\n", $name, var_export($info, true));
		if (isset($known_handler[$name]))
			unset($known_handler[$name]);
	}

	foreach ($known_handler as $name => $is_buildin)
		if ($is_buildin)
			printf("[004] Buildin handler '%s' not found\n", $name);

	$handler2 = mysqlnd_qc_get_available_handlers();
	if ($handler != $handler2) {
		printf("[005] Results should be identical\n");
		var_dump($handler);
		var_dump($handler2);
	}

	print "done!";
?>
--EXPECTF--
done!