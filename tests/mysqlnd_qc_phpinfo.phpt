--TEST--
phpinfo() - mysqlnd_qc entry
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	ob_start();
	phpinfo();
	$info = ob_get_contents();
	ob_end_clean();

	$expected = array(
		'mysqlnd_qc support' => 'enabled',
		'Mysqlnd Query Cache (mysqlnd_qc)' => '1.0.2alpha (100002)',
		'Cache by default?' => 'No (overruled by user handlers: No)',
		'cache_miss' => '0',
		'cache_put' => '0',
		'default' => 'default',
		'user'	=> 'enabled',
		'object' => 'enabled',
		'nop' => 'enabled',
	);

	foreach ($expected as $key => $example_value) {
		if (!stristr($info, $key)) {
			printf("'%s' not found, should show value like '%s'\n", $key, $example_value);
		}
	}

	print "done!";
?>
--EXPECTF--
done!