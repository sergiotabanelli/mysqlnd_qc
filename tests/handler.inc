<?php
	$env_handler = strtolower(getenv("MYSQLND_QC_HANDLER") ? getenv("MYSQLND_QC_HANDLER") : "default");

	$available_handler = array();
	$handler = mysqlnd_qc_get_available_handlers();
	foreach ($handler as $k => $v)
		$available_handler[strtolower($k)] = $k;

	switch ($env_handler) {
		case "userp":
			require_once("connect.inc");
			include_once("user_handler_helper.inc");
			uh_set_mode(MYSQLND_QC_UH_MODE_PERSISTENT);
			ini_set("memory_limit", "256M");
			if (true !== ($tmp = mysqlnd_qc_set_is_select("is_select")))
				printf("[h001] '%s' is_select rejected, got %s\n", "is_select", var_export($tmp, true));
			if (true !== ($tmp = mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "update_stats", "get_stats", "clear_cache")))
				printf("[h002] '%s' handler rejected, got %s\n", $env_handler, var_export($tmp, true));
			break;
		case "user":
			include_once("user_handler_helper.inc");
			uh_set_mode(MYSQLND_QC_UH_MODE_NON_PERSISTENT);
			ini_set("memory_limit", "256M");
			if (true !== ($tmp = mysqlnd_qc_set_is_select("is_select")))
				printf("[h003] '%s' is_select rejected, got %s\n", "is_select", var_export($tmp, true));
			if (true !== ($tmp = mysqlnd_qc_set_user_handlers("get_hash", "find", "return_to_cache", "add", "update_stats", "get_stats", "clear_cache")))
				printf("[h004] '%s' handler rejected, got %s\n", $env_handler, var_export($tmp, true));
			break;

		default:
			if (!isset($available_handler[$env_handler]))
				die(sprintf("skip Handler '%s' not available. Environment variable MYSQLND_QC_HANDLER set to '%s'.", $env_handler, getenv("MYSQLND_QC_HANDLER")));
			if (true !== ($tmp = mysqlnd_qc_set_is_select("mysqlnd_qc_default_query_is_select")))
				printf("[h005] '%s' is_select rejected, got %s\n", "mysqlnd_qc_default_query_is_select", var_export($tmp, true));
			if (true !== ($tmp = mysqlnd_qc_set_storage_handler($env_handler)))
				printf("[h006] '%s' handler rejected, got %s\n", $env_handler, var_export($tmp, true));
			break;
	}
?>