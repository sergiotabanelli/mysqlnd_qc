--TEST--
Constants exported by the query cache plugin
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
mysqlnd_qc.use_request_time=0
--FILE--
<?php
	$expected = array(
		'MYSQLND_QC_VERSION' 		=> array("type" => "string", "value" => "1.2.0-alpha"),
		'MYSQLND_QC_VERSION_ID'		=> array("type" => "integer", "value" => 10200),
		'MYSQLND_QC_ENABLE_SWITCH' 	=> array("type" => "string", "value" => "qc=on"),
		'MYSQLND_QC_DISABLE_SWITCH' => array("type" => "string", "value" => "qc=off"),
		'MYSQLND_QC_TTL_SWITCH' 	=> array("type" => "string", "value" => "qc_ttl="),
		'MYSQLND_QC_SERVER_ID_SWITCH' 	=> array("type" => "string", "value" => "qc_sid="),
		'MYSQLND_QC_CONDITION_META_SCHEMA_PATTERN' => array("type" => "integer", "value" => NULL),
	);
	$unexpected = array();
	$constants = get_defined_constants();
	foreach ($constants as $name => $value) {
		if (substr($name, 0, 10) == "MYSQLND_QC") {
			if (isset($expected[$name])) {
				if (gettype($value) != $expected[$name]['type']) {
					printf("[001] Expecting %s to be of type %s but got '%s'",
						$name, $expected[$name]['type'], gettype($value));
				}
				if (!is_null($expected[$name]['value']) && ($value != $expected[$name]['value'])) {
					printf("[002] Expecting %s to be be qual to  %s but got '%s'",
						$name, $expected[$name]['value'], $value);
				}
				unset($expected[$name]);
			} else {
				$unexpected[$name] = array('type' => gettype($value), 'value' => $value);
			}
		}
	}

	if (!empty($expected)) {
		printf("[003] Dumping list of missing constants\n");
		var_dump($expected);
	}

	if (!empty($unexpected)) {
		printf("[004] Dumping list of unexpected constants\n");
		var_dump($unexpected);
	}

	print "done!";
?>
--EXPECTF--
done!