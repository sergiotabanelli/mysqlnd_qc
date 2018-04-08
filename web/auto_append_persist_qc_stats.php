<?php
/**
* Example of an auto_append file to persist QC statistics
*
* NOTE: You must setup SQL tables manually.See setup()
* for the SQL statement.
*/

namespace qc_auto_append {

	/**
	* Where to store the collected data
	*
	* This example only implements 'mysql' to demo a fire and forget approach.
	* In real-life you may want to use other storage, for example, log files
	* APC, Memcache or send the data over the network to a monitor application.
	*/
	define("STORAGE", "mysql");

	define("MYSQL_USER", 	"root");
	define("MYSQL_PASS", 	"root");
	define("MYSQL_HOST", 	"127.0.0.1");
	define("MYSQL_DB", 		"test");
	define("MYSQL_SOCKET", 	"/tmp/mysql.sock");
	define("MYSQL_PORT", 	NULL);
	define("MYSQL_PREFIX",	"qc_auto_append_");

	define("USE_EXCEPTIONS", true);
	define("RECORD_HANDLER_STATS", true);
	define("RECORD_QUERY_LOG", true);
	define("RECORD_QUERY_LOG_ELIGIBLE_FOR_CACHING_ONLY", false);
	define("RECORD_NORMALIZED_QUERY_LOG", true);

	/* Main */

	// RUN THE SETUP MANUALLY, ONCE
    // setup();
	// sleep(1);
	collect();

	/* Function definitions */

	function connect() {
		return mysqli_connect(MYSQL_HOST, MYSQL_USER, MYSQL_PASS, MYSQL_DB, (int)MYSQL_PORT, MYSQL_SOCKET);
	} // end func connect

	function setup() {

		/* Let's use tablename_prefix to make cut&paste easier */
		$sql = <<<EOT
DROP TABLE IF EXISTS `tablename_prefix_statistics_record`;
CREATE TABLE `tablename_prefix_statistics_record` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `pid` int(10) unsigned NOT NULL,
  `request_counter` int(10) unsigned NOT NULL,
  `process_hash` int(10) unsigned NOT NULL,
  `pid_process_hash` VARCHAR(20) NOT NULL,
  `ini_collect_statistics` enum('on','off') NOT NULL DEFAULT 'off',
  `ini_time_statistics` enum('on','off') NOT NULL DEFAULT 'on',
  `ini_cache_no_table` enum('on','off') NOT NULL DEFAULT 'off',
  `ini_slam_defense` enum('on','off') NOT NULL DEFAULT 'off',
  `ini_collect_query_trace` enum('on','off') NOT NULL DEFAULT 'off',
  `ini_collect_normalized_query_trace` enum('on','off') NOT NULL DEFAULT 'off',
  `ini_query_trace_bt_depth` enum('on','off') NOT NULL DEFAULT 'off',
  `handler` varchar(127) NOT NULL,
  `handler_version` varchar(127) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_create` (`created`),
  KEY `idx_pid_group` (`pid_process_hash`, `created`, `request_counter`, `pid`, `id`),
  KEY `idx_handler` (`handler`,`handler_version`)
);

DROP TABLE IF EXISTS `tablename_prefix_core_statistics`;
CREATE TABLE `tablename_prefix_core_statistics` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `fk_statistics_record_id` int(10) unsigned NOT NULL,
  `cache_hit` int(10) unsigned DEFAULT NULL,
  `cache_miss` int(10) unsigned DEFAULT NULL,
  `cache_put` int(10) unsigned DEFAULT NULL,
  `query_should_cache` int(10) unsigned DEFAULT NULL,
  `query_should_not_cache` int(10) unsigned DEFAULT NULL,
  `query_not_cached` int(10) unsigned DEFAULT NULL,
  `query_could_cache` int(10) unsigned DEFAULT NULL,
  `query_found_in_cache` int(10) unsigned DEFAULT NULL,
  `query_uncached_other` int(10) unsigned DEFAULT NULL,
  `query_uncached_no_table` int(10) unsigned DEFAULT NULL,
  `query_uncached_use_result` int(10) unsigned DEFAULT NULL,
  `query_aggr_run_time_cache_hit` bigint(20) unsigned DEFAULT NULL,
  `query_aggr_run_time_cache_put` bigint(20) unsigned DEFAULT NULL,
  `query_aggr_run_time_total` bigint(20) unsigned DEFAULT NULL,
  `query_aggr_store_time_cache_hit` bigint(20) unsigned DEFAULT NULL,
  `query_aggr_store_time_cache_put` bigint(20) unsigned DEFAULT NULL,
  `query_aggr_store_time_total` bigint(20) unsigned DEFAULT NULL,
  `receive_bytes_recorded` bigint(20) unsigned DEFAULT NULL,
  `receive_bytes_replayed` bigint(20) unsigned DEFAULT NULL,
  `send_bytes_recorded` bigint(20) unsigned DEFAULT NULL,
  `send_bytes_replayed` bigint(20) unsigned DEFAULT NULL,
  `slam_stale_refresh` int(10) unsigned DEFAULT NULL,
  `slam_stale_hit` int(10) unsigned DEFAULT NULL,
  `derived_cache_hit_ratio` float unsigned DEFAULT NULL,
  `derived_cache_efficiency` float unsigned DEFAULT NULL,
  `derived_time_saved` float unsigned DEFAULT NULL,
  `derived_traffic_saved` float unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_statistics_record_id` (`fk_statistics_record_id`),
  KEY `idx_cache_hit_ratio` (`derived_cache_hit_ratio`),
  KEY `idx_cache_efficiency` (`derived_cache_efficiency`),
  KEY `idx_time_saved` (`derived_time_saved`),
  KEY `idx_traffic_saved` (`derived_traffic_saved`)
);

DROP TABLE IF EXISTS `tablename_prefix_handler_statistics`;
CREATE TABLE `tablename_prefix_handler_statistics` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `fk_statistics_record_id` int(10) unsigned NOT NULL,
  `stats_key` varchar(1024) NOT NULL,
  `rows` int(10) unsigned DEFAULT NULL,
  `stored_size` int(10) unsigned DEFAULT NULL,
  `cache_hits` int(10) unsigned DEFAULT NULL,
  `run_time` int(10) unsigned DEFAULT NULL,
  `store_time` int(10) unsigned DEFAULT NULL,
  `min_run_time` int(10) unsigned DEFAULT NULL,
  `avg_run_time` int(10) unsigned DEFAULT NULL,
  `max_run_time` int(10) unsigned DEFAULT NULL,
  `min_store_time` int(10) unsigned DEFAULT NULL,
  `avg_store_time` int(10) unsigned DEFAULT NULL,
  `max_store_time` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
);

DROP TABLE IF EXISTS `tablename_prefix_query_trace`;
CREATE TABLE `tablename_prefix_query_trace` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `fk_statistics_record_id` int(10) unsigned NOT NULL,
  `query_string` varchar(1024) DEFAULT NULL,
  `origin` blob,
  `run_time` int(10) unsigned DEFAULT NULL,
  `store_time` int(10) unsigned DEFAULT NULL,
  `eligible_for_caching` enum('true','false') DEFAULT NULL,
  `no_table` enum('true','false') DEFAULT NULL,
  `was_added` enum('true','false') DEFAULT NULL,
  `was_already_in_cache` enum('true','false') DEFAULT NULL,
  PRIMARY KEY (`id`)
);

DROP TABLE IF EXISTS `tablename_prefix_normalized_query_log`;
CREATE TABLE `tablename_prefix_normalized_query_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `fk_statistics_record_id` int(10) unsigned NOT NULL,
  `query_string` varchar(1024) DEFAULT NULL,
  `min_run_time` int(10) unsigned DEFAULT NULL,
  `avg_run_time` int(10) unsigned DEFAULT NULL,
  `max_run_time` int(10) unsigned DEFAULT NULL,
  `min_store_time` int(10) unsigned DEFAULT NULL,
  `avg_store_time` int(10) unsigned DEFAULT NULL,
  `max_store_time` int(10) unsigned DEFAULT NULL,
  `eligible_for_caching` enum('true','false') DEFAULT NULL,
  `occurences` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
);
EOT;
		$sql = str_replace("tablename_prefix_", MYSQL_PREFIX, $sql);

		$link = connect();
		if (!$link->multi_query($sql) || $link->errno) {
			$error = sprintf("[%s:%d] [%d] %s - %s\n", __FILE__, __LINE__, $link->errno, $link->error, $sql);
			if (USE_EXCEPTIONS)
				throw new \Exception($error);
			else
				echo $error;
			return false;
		}

		return true;
	} // end func setup()

	function collect() {

		$link = connect();

		$pid = ($tmp = getmypid()) ? $tmp : NULL;

		$cache_info = mysqlnd_qc_get_cache_info();
		$handler = $cache_info['handler'];
		$handler_version = $cache_info['handler_version'];

		$stats = mysqlnd_qc_get_core_stats();

		$sql = sprintf("INSERT INTO %sstatistics_record(
			pid, request_counter, process_hash, pid_process_hash,
			ini_collect_statistics, ini_time_statistics,
			ini_cache_no_table, ini_slam_defense,
			ini_collect_query_trace,
			ini_collect_normalized_query_trace,
			ini_query_trace_bt_depth,
			handler, handler_version
		) VALUES (
			%d, %d, %d, '%s',
			'%s', '%s',
			'%s', '%s',
			'%s',
			'%s',
			'%s',
			'%s', '%s')",
			MYSQL_PREFIX,
			$pid,
			$stats['request_counter'],
			$stats['process_hash'],
			$pid . $stats['process_hash'],
			ini_get('mysqlnd_qc.collect_statistics') ? 'on' : 'off',
			ini_get('mysqlnd_qc.time_statistics') ? 'on' : 'off',
			ini_get('mysqlnd_qc.cache_no_table') ? 'on' : 'off',
			ini_get('mysqlnd_qc.slam_defense') ? 'on' : 'off',
			ini_get('mysqlnd_qc.collect_query_trace') ? 'on' : 'off',
			ini_get('mysqlnd_qc.collect_normalized_query_trace') ? 'on' : 'off',
			ini_get('mysqlnd_qc.query_trace_bt_depth'),
			$link->real_escape_string($handler),
			$link->real_escape_string($handler_version)
		);

		if (!$link->query($sql)) {
			$error = sprintf("[%s:%d] [%d] %s - %s\n", __FILE__, __LINE__, $link->errno, $link->error, $sql);
			if (USE_EXCEPTIONS)
				throw new \Exception($error);
			else
				echo $error;
			return false;
		}
		$fk_statistics_record_id = $link->insert_id;


		$total_queries = $stats['query_should_cache'] + $stats['query_should_not_cache'];

		$sql = sprintf("INSERT INTO %score_statistics(
			fk_statistics_record_id,
			cache_hit, cache_miss, cache_put, query_should_cache,
			query_should_not_cache, query_not_cached,
			query_could_cache, query_found_in_cache,
			query_uncached_other, query_uncached_no_table,
			query_uncached_use_result,
			query_aggr_run_time_cache_hit,
			query_aggr_run_time_cache_put,
			query_aggr_run_time_total,
			query_aggr_store_time_cache_hit,
			query_aggr_store_time_cache_put,
			query_aggr_store_time_total,
			receive_bytes_recorded,
			receive_bytes_replayed,
			send_bytes_recorded,
			send_bytes_replayed,
			slam_stale_refresh,
			slam_stale_hit,
			derived_cache_hit_ratio,
			derived_cache_efficiency,
			derived_time_saved,
			derived_traffic_saved
		) VALUES (
			%d,
			%d, %d, %d, %d,
			%d, %d,
			%d, %d,
			%d, %d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%d,
			%f,
			%f,
			%f,
			%f
		)",
			MYSQL_PREFIX,
			$fk_statistics_record_id,
			$stats['cache_hit'], $stats['cache_miss'], $stats['cache_put'], $stats['query_should_cache'],
			$stats['query_should_not_cache'], $stats['query_not_cached'],
			$stats['query_could_cache'], $stats['query_found_in_cache'],
			$stats['query_uncached_other'], $stats['query_uncached_no_table'],
			$stats['query_uncached_use_result'],
			$stats['query_aggr_run_time_cache_hit'],
			$stats['query_aggr_run_time_cache_put'],
			$stats['query_aggr_run_time_total'],
			$stats['query_aggr_store_time_cache_hit'],
			$stats['query_aggr_store_time_cache_put'],
			$stats['query_aggr_store_time_total'],
			$stats['receive_bytes_recorded'],
			$stats['receive_bytes_replayed'],
			$stats['send_bytes_recorded'],
			$stats['send_bytes_replayed'],
			$stats['slam_stale_refresh'],
			$stats['slam_stale_hit'],
			($stats['cache_hit'] == 0) ? 0 : (100 - ((100 / $stats['cache_hit']) * $stats['cache_miss'])),
			($total_queries == 0) ? 0 : ((100 / $total_queries) * $stats['cache_hit']),
			($stats['query_aggr_run_time_cache_put'] + $stats['query_aggr_store_time_cache_put']) * $stats['cache_hit'] / 1000000,
			($stats['receive_bytes_replayed'] + $stats['send_bytes_replayed']) / 1024 / 1024
		);

		if (!$link->query($sql)) {
			$error = sprintf("[%s:%d] [%d] %s - %s\n", __FILE__, __LINE__, $link->errno, $link->error, $sql);
			if (USE_EXCEPTIONS)
				throw new \Exception($error);
			else
				echo $error;
			return false;
		}

		if (RECORD_HANDLER_STATS && !empty($cache_info['data'])) {

			switch (strtolower($handler)) {
				case "apc":
				case "default":
					foreach ($cache_info['data'] as $key => $data) {
						$stats = $data['statistics'];
						$sql = sprintf("INSERT INTO %shandler_statistics(
							fk_statistics_record_id,
							stats_key,
							rows, stored_size, cache_hits,
							run_time, store_time,
							min_run_time, avg_run_time, max_run_time,
							min_store_time, avg_store_time, max_store_time
						) VALUES (
							%d,
							'%s',
							%d, %d, %d,
							%d, %d,
							%d, %d, %d,
							%d, %d, %d
						)", MYSQL_PREFIX,
							$fk_statistics_record_id,
							$link->real_escape_string($key),
							isset($stats['rows']) ? $stats['rows'] : 0,
							isset($stats['stored_size']) ? $stats['stored_size'] : 0,
							$stats['cache_hits'],
							$stats['run_time'], $stats['store_time'],
							$stats['min_run_time'], $stats['avg_run_time'], $stats['max_run_time'],
							$stats['min_store_time'], $stats['avg_store_time'], $stats['max_store_time']
						);

						if (!$link->query($sql)) {
							$error = sprintf("[%s:%d] [%d] %s - %s\n", __FILE__, 	__LINE__, $link->errno, $link->error, $sql);
							if (USE_EXCEPTIONS)
								throw new \Exception($error);
							else
								echo $error;
							return false;
						}
					}
				break;
			}
		}

		if (RECORD_QUERY_LOG) {
			$query_log = mysqlnd_qc_get_query_trace_log();
			foreach ($query_log as $k => $bt) {
				if (RECORD_QUERY_LOG_ELIGIBLE_FOR_CACHING_ONLY && !$bt['eligible_for_caching'])
					continue;

				$sql = sprintf("
				INSERT INTO %squery_trace(
					fk_statistics_record_id,
					query_string,
					origin,
					run_time,
					store_time,
					eligible_for_caching,
					no_table,
					was_added,
					was_already_in_cache
				) VALUES (
					%d,
					'%s',
					'%s',
					%d,
					%d,
					'%s',
					'%s',
					'%s',
					'%s'
				)
				",  MYSQL_PREFIX,
					$fk_statistics_record_id,
					(true) ? '' : substr($link->real_escape_string($bt['query']), 0, 1024),
					(true) ? '' : substr($link->real_escape_string($bt['origin']), 0, 8192),
					$bt['run_time'],
					$bt['store_time'],
					($bt['eligible_for_caching']) ? 'true' : 'false',
					($bt['no_table']) ? 'true' : 'false',
					($bt['was_added']) ? 'true' : 'false',
					($bt['was_already_in_cache']) ? 'true' : 'false'
				);

				if (!$link->query($sql)) {
					$error = sprintf("[%s:%d] [%d] %s - %s\n", __FILE__, __LINE__, $link->errno, $link->error, $sql);
					if (USE_EXCEPTIONS)
						throw new \Exception($error);
					else
						echo $error;
					return false;
				}
			}
		}

		if (RECORD_NORMALIZED_QUERY_LOG) {
			$query_log = mysqlnd_qc_get_normalized_query_trace_log();
			foreach ($query_log as $k => $bt) {

				$sql = sprintf("
				INSERT INTO %snormalized_query_log(
					fk_statistics_record_id,
					query_string,
					min_run_time,
					avg_run_time,
					max_run_time,
					min_store_time,
					avg_store_time,
					max_store_time,
					eligible_for_caching,
					occurences
				) VALUES (
					%d,
					'%s',
					%d,
					%d,
					%d,
					%d,
					%d,
					%d,
					'%s',
					%d
				)
				",  MYSQL_PREFIX,
					$fk_statistics_record_id,
					(true) ? '' : substr($link->real_escape_string($bt['query']), 0, 1024),
					$bt['min_run_time'],
					$bt['avg_run_time'],
					$bt['max_run_time'],
					$bt['min_store_time'],
					$bt['avg_store_time'],
					$bt['max_store_time'],
					($bt['eligible_for_caching']) ? 'true' : 'false',
					$bt['occurences']
				);

				if (!$link->query($sql)) {
					$error = sprintf("[%s:%d] [%d] %s - %s\n", __FILE__, __LINE__, $link->errno, $link->error, $sql);
					if (USE_EXCEPTIONS)
						throw new \Exception($error);
					else
						echo $error;
					return false;
				}
			}
		}

	} // end func collect
}
?>