<?php
/**
* Functions of the cache monitor
*
* Functions that generate HTML output. Used allover.
*
* Note: this collection of scripts does not aim to be a "modern"
* PHP web application. It is a collection of stuff that proved
* handy during the development of the MySQLnd Query Cache. No more!
*/

// require_once(dirname(__FILE__) . "/../tests/table.inc");
// ini_set("mysqlnd_qc.cache_no_table", true);
// $link->query("/*qc=on*/SELECT SLEEP(1)")->fetch_all();

require_once("mysqlnd_qc_monitor_config_inc.php");

/*
	Utility code
*/
function qc_get_connection() {
	return mysqli_connect(QCM_MYSQL_HOST, QCM_MYSQL_USER, QCM_MYSQL_PASS, QCM_MYSQL_DB, (int)QCM_MYSQL_PORT, QCM_MYSQL_SOCKET);
} // end func qc_get_connection


/*
	Stuff to be shown as a small box, e.g. on the left side of the screen
*/

/**
* Prints auto append (MySQL storage) summary  as a HTML table suitable for the left side box area
*
*/
function qc_print_box_append_summary() {

	$color_idx = 0;

	$link = qc_get_connection();
	if (!$res = $link->query("SELECT COUNT(*) AS _num, MIN(created) AS _min_created, MAX(created) AS _max_created FROM qc_auto_append_statistics_record"))
		throw new Exception("[%d] %s\n", $link->errno, $link->error);

	$row = $res->fetch_assoc();
	$res->free();

	?>
<table class="box">
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>Web requests</td>
		<td align="right"><?php print number_format($row['_num']); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>First record</td>
		<td align="right"><?php print qc_db_date_format($row['_min_created']); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>Last record</td>
		<td align="right"><?php print qc_db_date_format($row['_max_created']); ?></td>
	</tr>
</table>
	<?php

} // end func qc_print_box_append_summary


function qc_db_date_format($date) {
	return date("d.m.Y H:i:s", strtotime($date));
}


/**
* Prints QC and handler versions as a HTML table suitable for the left side box area
*
*/
function qc_print_box_version_list() {
	$color_idx = 0;
	?>
<table class="box">
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>QC Version</td>
		<td align="right"><?php print phpversion("mysqlnd_qc"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>PHP Version</td>
		<td align="right"><?php print phpversion(); ?></td>
	</tr>
	<?php
	$handler = mysqlnd_qc_get_available_handlers();
	foreach ($handler as $name => $details) {
		?>
		<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
			<td><?php printf("Handler '%s'", htmlspecialchars($name)); ?></td>
			<td align="right"><?php print htmlspecialchars($details['version']); ?></td>
		</tr>
		<?php
	} // foreach ($handler as $name => $details)
	?>
</table>
	<?php
} // end func qc_print_box_version_list


/**
* Prints the active storage handler as a HTML table suitable for the left side box area
*
*/
function qc_print_box_active_handler_summary() {
	$color_idx = 0;
	$info = mysqlnd_qc_get_cache_info();
	?>
<table class="box">
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>Handler</td>
		<td align="right"><?php print htmlspecialchars($info['handler']); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>Version</td>
		<td align="right"><?php print htmlspecialchars($info['handler_version']); ?></td>
	</tr>
</table>
	<?php
} // end func qc_print_box_active_handler_summary


/**
* Prints relevant PHP configuration settings as a HTML table suitable for the left side box area
*
*/
function qc_print_box_ini_settings_summary() {
	$handler = mysqlnd_qc_get_available_handlers();
	$color_idx = 0;

	?>
<table class="box">
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.enable_qc</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.enable_qc"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.ttl</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.ttl"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.cache_by_default</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.cache_by_default"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.cache_no_table</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.cache_no_table"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.slam_defense</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.slam_defense"); ?></td>
	</tr>

	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.collect_statistics</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.collect_statistics"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.time_statistics</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.time_statistics"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.collect_query_trace</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.collect_query_trace"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.query_trace_bt_depth</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.query_trace_bt_depth"); ?></td>
	</tr>
	<?php if (function_exists("mysqlnd_qc_get_normalized_query_trace_log")) { ?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.collect_normalized_query_trace</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.collect_normalized_query_trace"); ?></td>
	</tr>
	<?php } ?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.use_request_time</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.use_request_time"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.std_data_copy</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.std_data_copy"); ?></td>
	</tr>
	<?php
	if (isset($handler['APC'])) { ?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.apc_prefix</td>
		<td align="right">"<?php print ini_get("mysqlnd_qc.apc_prefix"); ?>"</td>
	</tr>
	<?php } ?>
	<?php if (isset($handler['MEMCACHE'])) { ?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.memc_server</td>
		<td align="right">"<?php print ini_get("mysqlnd_qc.memc_server"); ?>"</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>mysqlnd_qc.memc_port</td>
		<td align="right"><?php print ini_get("mysqlnd_qc.memc_port"); ?></td>
	</tr>
	<?php } ?>
</table>
	<?php
	if (isset($handler['APC']) && extension_loaded("apc")) {
	$color_idx = 0;
?>
	<b>Related APC configuration</v>
	<table class="box">
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>apc.use_request_time</td>
		<td align="right"><?php print ini_get("apc.use_request_time"); ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>apc.slam_defense</td>
		<td align="right"><?php print ini_get("apc.slam_defense"); ?></td>
	</tr>
	</table>
<?php
	} // end if (isset($handler['APC']) && extension_loaded("apc"))

} // end func qc_print_box_ini_settings_summary


/**
* Prints relevant QC constants as a HTML table suitable for the left side box area
*
*/
function qc_print_box_constants() {
	$constants = get_defined_constants();
	foreach ($constants as $k => $v)
		if ("MYSQLND_QC" != substr($k, 0, 10))
			unset($constants[$k]);
	asort($constants);
	$color_idx = 0;
	?>
<table class="box">
	<?php
	foreach ($constants as $k => $v) {
	?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td><?php print htmlspecialchars($k); ?></td>
		<td align="right">"<?php print htmlspecialchars($v); ?>"</td>
	</tr>
	<?php
	} // end foreach ($constants as $k => $v)
	?>
</table>
	<?php
} // end func qc_print_box_active_handler_summary


/**
* Prints a cache contents summary as a HTML table suitable for the left side box area
*
* Output depends on the active handler.
* mysqlnd_qc_get_cache_info() is used to provide the data. Only APC and default
* report details through this function. There may be no data if you are using
* a different cache storage handler.
*/
function qc_print_box_cache_handler_summary() {
	$info = mysqlnd_qc_get_cache_info();
	$color_idx = 0;
	?>
<table class="box">
	<tr>
		<td colspan="2" bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<p>
		The below figures are based
		on statistics provided by the storage handler.
		Some handler do not provide statistics.
		Some handler provide per-process statistics (e.g. Default).
		Others do provide per-machine (multiple processes) statistics (e.g. APC).
		Keep this in mind when interpreting the figures.
		</p>
		</td>
	</tr>
	<?php
	switch (strtolower($info['handler'])) {
		case 'default':
			$scope = 'process';
			break;
		case 'apc':
			$scope = 'machine (multiple processes)';
			break;
		case 'memcache':
			$scope = 'global (multiple processes, multiple machines)';
			break;
		default:
			$scope = 'unknown';
			break;
	}
	?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td><b>Scope of figures</b></td>
		<td align="right"><b><?php print $scope; ?></b></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>Number of cache entries</td>
		<td align="right"><?php print htmlspecialchars($info['num_entries']); ?></td>
	</tr>
	<?php
	if (empty($info['data'])) {
	?>
	<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>Details</td>
		<td align="right">n/a with handler <?php print $info['handler']; ?></td>
	</tr>
	<?php
	} else {
		// else (empty($info['data']))
		switch (strtolower($info['handler'])) {
			case "apc":
			case "default":
				$total_hits = 0;
				$total_rows = 0;
				$total_bytes = "n/a";
				$have_timings =  (1 == ini_get("mysqlnd_qc.time_statistics"));
				if ($have_timings) {
					$time_saved = 0;
					$avg_speedup = 0;
					$cached = $uncached = 0;
				} else {
					$time_saved = NULL;
					$avg_speedup = NULL;
				}
				foreach ($info['data'] as $key => $details) {
					$total_hits += $details['statistics']['cache_hits'];
					$total_rows += $details['statistics']['rows'];
					if ($have_timings) {
						$cached += ((($details['statistics']['avg_run_time'] + $details['statistics']['avg_store_time']) * $details['statistics']['cache_hits']) + ($details['statistics']['run_time'] + $details['statistics']['store_time'])) / 1000000;
						$uncached += (($details['statistics']['run_time'] + $details['statistics']['store_time']) * ($details['statistics']['cache_hits'] + 1)) / 1000000;
					}
				}
				if ($have_timings) {
					$time_saved = $uncached - $cached;
					$avg_speedup = (100 / $cached) * $uncached;
				}
				break;
			default:
				$total_hits = "n/a";
				$total_rows = "n/a";
				$total_bytes = "n/a";
				$time_saved = NULL;
				$avg_speedup = NULL;
				break;
		}
		?>
		<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
			<td>Hits</td>
			<td align="right"><?php print $total_hits; ?></td>
		</tr>
		<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
			<td>Bytes</td>
			<td align="right"><?php print $total_bytes; ?></td>
		</tr>
		<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
			<td>Rows</td>
			<td align="right"><?php print $total_rows; ?></td>
		</tr>
		<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
			<td>Time saved</td>
			<td align="right"><?php echo (NULL == $time_saved) ? "n/a" : sprintf("%2.4fs", $time_saved); ?></td>
		</tr>
		<tr bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
			<td>Avg speed-up</td>
			<td align="right"><?php echo (NULL == $avg_speedup) ? "n/a" : sprintf("%.2f%%", $avg_speedup); ?></td>
		</tr>
		<?php
	?>
	<?php
	} // if (empty($info['data']))
	?>
</table>
	<?php
}

/*
Functions suitable for the "main" contents of the screen
*/

define("QCM_SORT_HITS", 1);
define("QCM_SORT_SPEEDUP", 2);
define("QCM_SORT_TIME_SAVED", 3);
define("QCM_SORT_SPEEDUP_NOT_AGGR", 4);
define("QCM_SORT_ORDER_ASC", 1);
define("QCM_SORT_ORDER_DESC", 2);

/**
* Prints a cache contents details as a HTML table suitable for the main contents area
*
* Output depends on the active handler.
* mysqlnd_qc_get_cache_info() is used to provide the data. Only APC and default
* report details through this function. There may be no data if you are using
* a different cache storage handler.
*/
function qc_print_main_cache_handler_details($limit, $sort_by = QCM_SORT_TIME_SAVED, $sort_order = QCM_SORT_ORDER_DESC) {
	$info = mysqlnd_qc_get_cache_info();
	$color_idx = 0;

	switch (strtolower($info['handler'])) {
		case 'default':
			$scope = 'process';
			break;
		case 'apc':
			$scope = 'machine (multiple processes)';
			break;
		case 'memcache':
			$scope = 'global (multiple processes, multiple machines)';
			break;
		default:
			$scope = 'unknown';
			break;
	}
	?>
<table class="box">
	<tr>
		<td colspan="13" bgcolor="<?php print ($color_idx++ %2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<p><b>Scope: <?php print $scope; ?></b></p>
		</td>
	</tr>
	<tr>
		<th>Hits</th>
		<th>Speed-up (aggr.)</th>
		<th>Speed-up</th>
		<th>Time saved</th>
		<th>Rows</th>
		<th>Org run time</th>
		<th>Avg run time</th>
		<th>Min run time</th>
		<th>Max run time</th>
		<th>Org store time</th>
		<th>Avg store time</th>
		<th>Min store time</th>
		<th>Max store time</th>
	</tr>
	<?php
	if (empty($info['data'])) {
	?>
	<tr bgcolor="<?php print QCM_TD_LIST_COLOR1; ?>">
		<td colspan="13">n/a - cache is empty</td>
	</tr>
	<?php
	} else {
		// else (empty($info['data'])) = not empty
		$rows = qc_compute_cache_content_stats($info, $sort_by, $sort_order);

		if (empty($rows)) {
		?>
		<tr bgcolor="<?php print QCM_TD_LIST_COLOR1; ?>">
			<td colspan="13">n/a - handler does not provide information</td>
		</tr>
		<?php
		} else { // else empty($rows)
			$i = 1;
			foreach ($rows as $key => $data) {
				$i++;
				if ($i > $limit)
					break;
			?>
			<tr bgcolor="<?php print ($i % 2) ? QCM_TD_LIST_COLOR1 : QCM_TD_LIST_COLOR0; ?>">
				<td><?php print ($data['hits'] === NULL) ? 'n/a' : $data['hits']; ?></td>
				<td><?php print ($data['speedup'] === NULL) ? 'n/a' : sprintf("%.2f%%", $data['speedup']); ?></td>
				<td><?php print ($data['speedup_not_aggr'] === NULL) ? 'n/a' : sprintf("%.2fx", $data['speedup_not_aggr']); ?></td>
				<td><?php print ($data['time_saved'] === NULL) ? 'n/a' : sprintf("%.3fs", $data['time_saved']); ?></td>
				<td><?php print ($data['rows'] === NULL) ? 'n/a' : $data['rows']; ?></td>
				<td><?php print ($data['org_run_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['org_run_time']); ?></td>
				<td><?php print ($data['avg_run_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['avg_run_time']); ?></td>
				<td><?php print ($data['min_run_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['min_run_time']); ?></td>
				<td><?php print ($data['max_run_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['max_run_time']); ?></td>
				<td><?php print ($data['org_store_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['org_store_time']); ?></td>
				<td><?php print ($data['avg_store_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['avg_store_time']); ?></td>
				<td><?php print ($data['min_store_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['min_store_time']); ?></td>
				<td><?php print ($data['max_store_time'] === NULL) ? 'n/a' : sprintf("%.6fs", $data['max_store_time']); ?></td>
			</tr>
			<?php
			if ($data['query'] !== NULL) {
			?>
			<tr bgcolor="<?php print ($i % 2) ? QCM_TD_LIST_COLOR1 : QCM_TD_LIST_COLOR0; ?>">
				<td></td>
				<td colspan="12"><code><?php print htmlspecialchars($data['query']); ?></code></td>
			</tr>
			<?php
			} // if ($data['query'] !== NULL)
			} // foreach ($rows as $key => $data) {
		} // if (empty($rows)) {
	} // if (empty($info['data']))
	?>
</table>
	<?php
} // end func qc_print_main_cache_handler_details

/**
* Streams cache contents as a CSV file
*
* The function makes use of mysqlnd_qc_get_cache_info(). Not every storage handler
* provides data through this function and the scope of the data varies by
* the storage handler.
*/
function qc_stream_cache_contents($limit, $sort_by = QCM_SORT_TIME_SAVED, $sort_order = QCM_SORT_ORDER_DESC) {
	$info = mysqlnd_qc_get_cache_info();
	$file_name = sprintf("qc_stats_%d.csv", time());

	header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
	header("Content-Description: File Transfer");
	header("Content-type: text/csv");
	header(sprintf("Content-Disposition: attachment; filename=%s", $file_name));
	header("Expires: 0");
?>"Row";"Cache Key";"Hits";"Speed-up (aggr., percent)";"Speed-up (absolut)";"Time saved (s)";"Rows";"Org run time (s)";"Avg run time (s)";"Min run time (s)";"Max run time (s)";"Org store time (s)";"Avg store time (s)";"Min store time (s)";"Max store time (s)"
<?php
		$rows = qc_compute_cache_content_stats($info, $sort_by, $sort_order);
		$i = 1;
		foreach ($rows as $key => $data) {
			if ($limit > 0 && $i > $limit)
				break;

			printf("\n");
			printf('"%d";', $i);
			printf('"%s";', trim(str_replace('"', '\\"', $data['query'])));
			printf('"%d";', $data['hits']);
			printf('"%f";', $data['speedup']);
			printf('"%f";', $data['speedup_not_aggr']);
			printf('"%f";', $data['time_saved']);
			printf('"%d";', $data['rows']);
			printf('"%f";', $data['org_run_time']);
			printf('"%f";', $data['avg_run_time']);
			printf('"%f";', $data['min_run_time']);
			printf('"%f";', $data['max_run_time']);
			printf('"%f";', $data['org_store_time']);
			printf('"%f";', $data['avg_store_time']);
			printf('"%f";', $data['min_store_time']);
			printf('"%f"', $data['max_store_time']);

			$i++;
		} // foreach ($rows as $key => $data)
}

/**
* Utility function to compute cache content statistics for display
*
* @protected Kind of
*/
function qc_compute_cache_content_stats($info, $sort_by, $sort_order) {

	$rows = array();

	if (!empty($info['data'])) {

		switch (strtolower($info['handler'])) {
			case "apc":
				$total_hits = 0;
				$total_rows = 0;
				$total_bytes = "n/a";
				$have_timings =  (1 == ini_get("mysqlnd_qc.time_statistics"));
				if ($have_timings) {
					$time_saved = 0;
					$avg_speedup = 0;
					$cached = $uncached = 0;
				} else {
					$time_saved = NULL;
					$avg_speedup = NULL;
				}
				foreach ($info['data'] as $key => $details) {
					$total_hits += $details['statistics']['cache_hits'];
					$total_rows += $details['statistics']['rows'];
					if ($have_timings) {
						$cached += ((($details['statistics']['avg_run_time'] + $details['statistics']['avg_store_time']) * $details['statistics']['cache_hits']) + ($details['statistics']['run_time'] + $details['statistics']['store_time'])) / 100000;
						$uncached += (($details['statistics']['run_time'] + $details['statistics']['store_time']) * ($details['statistics']['cache_hits'] + 1)) / 100000;
					}

					if (false !== ($pos = strrpos($key, QCM_SQL_HINT)))
						$query = substr($key, $pos, strlen($key));
					else
						$query = NULL;

					$rows[$key] = array(
						'hits'				=> $details['statistics']['cache_hits'],
						'query'				=> $query,
						'speedup'			=> NULL,
						'speedup_not_aggr'	=> NULL,
						'time_saved'		=> NULL,
						'rows'				=> NULL,
						'org_run_time'		=> NULL,
						'avg_run_time'		=> NULL,
						'min_run_time'		=> NULL,
						'max_run_time'		=> NULL,
						'org_store_time'	=> NULL,
						'avg_store_time'	=> NULL,
						'min_store_time'	=> NULL,
						'max_store_time'	=> NULL,
					);
					if ($have_timings) {
						$speedup =  (100 / ((($details['statistics']['avg_run_time'] + $details['statistics']['avg_store_time']) * $details['statistics']['cache_hits']) + ($details['statistics']['run_time'] + $details['statistics']['store_time']))) * (($details['statistics']['run_time'] + $details['statistics']['store_time']) * ($details['statistics']['cache_hits'] + 1));
						$rows[$key]['speedup'] = $speedup;

						$speedup_not_aggr = ($details['statistics']['cache_hits']) ? (($details['statistics']['run_time'] + $details['statistics']['store_time']) / ($details['statistics']['avg_run_time'] + $details['statistics']['avg_store_time'])) : 0;
						$rows[$key]['speedup_not_aggr'] = $speedup_not_aggr;

						$saved = ((($details['statistics']['run_time'] + $details['statistics']['store_time']) * ($details['statistics']['cache_hits'] + 1)) - ((($details['statistics']['avg_run_time'] + $details['statistics']['avg_store_time']) * $details['statistics']['cache_hits']) + ($details['statistics']['run_time'] + $details['statistics']['store_time']))) / 1000000;
						$rows[$key]['time_saved'] = $saved;

						$rows[$key]['org_run_time'] = $details['statistics']['run_time'] / 1000000;
						$rows[$key]['avg_run_time'] = $details['statistics']['avg_run_time'] / 1000000;
						$rows[$key]['max_run_time'] = $details['statistics']['max_run_time'] / 1000000;
						$rows[$key]['min_run_time'] = $details['statistics']['min_run_time'] / 1000000;
						$rows[$key]['org_store_time'] = $details['statistics']['store_time'] / 1000000;
						$rows[$key]['avg_store_time'] = $details['statistics']['avg_store_time'] / 1000000;
						$rows[$key]['min_store_time'] = $details['statistics']['min_store_time'] / 1000000;
						$rows[$key]['max_store_time'] = $details['statistics']['max_store_time'] / 1000000;
					}
				}
				if ($have_timings) {
					$time_saved = $uncached - $cached;
					$avg_speedup = (100 / $cached) * $uncached;
				}

				break;
			default:
				/* leave rows empty */
				break;
		}

		if (!empty($rows)) {
			$sort_func = null;
			switch ($sort_order) {
				case QCM_SORT_ORDER_ASC:
					$sort_cmp = "<";
					break;
				case QCM_SORT_ORDER_DESC:
					$sort_cmp = ">";
					break;
			}

			switch ($sort_by) {
				case QCM_SORT_HITS:
					$sort_func = create_function('$a, $b', 'if ($a["hits"] == $b["hits"]) return 0; return ($a["hits"] ' . $sort_cmp . '$b["hits"]) ? -1 : 1; ');
					break;
				case QCM_SORT_SPEEDUP:
					$sort_func = create_function('$a, $b', 'if ($a["speedup"] == $b["speedup"]) return 0; return ($a["speedup"] ' . $sort_cmp . '$b["speedup"]) ? -1 : 1; ');
					break;
				case QCM_SORT_TIME_SAVED:
					$sort_func = create_function('$a, $b', 'if ($a["time_saved"] == $b["time_saved"]) return 0; return ($a["time_saved"] ' . $sort_cmp . '$b["time_saved"]) ? -1 : 1; ');
					break;
				default:
					break;
			}
			if (!is_null($sort_func))
				usort($rows, $sort_func);
		}
	}

	return $rows;
}


/**
* Prints a cache core statistics summary as a HTML table suitable for the main contents area
*
* Process scrop - core_stats()!
*/
function qc_print_main_core_stats_summary() {
	$stats = mysqlnd_qc_get_core_stats();
	$info = mysqlnd_qc_get_cache_info();
	$handler = strtolower($info['handler']);
	$color_idx = 0;
?>
<table class="box">
	<tr>
		<th>Statistic</th>
		<th>Value</th>
		<th>Formula</th>
	</tr>
	<?php
	if (empty($stats)) {
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td colspan="3">n/a - cache is empty</td>
	</tr>
	<?php
	} else {
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td><b>Cache hit ratio</b></td>
		<td><b><?php printf("%.2f%%", ($stats['cache_hit'] == 0) ? 0 : (100 - (100 / $stats['cache_hit']) * $stats['cache_miss'])); ?></b></td>
		<td>100 - ((100 / cache_hit) * cache_miss)</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td style="white-space: nowrap;"><b>Cache efficiency</b></td>
		<td><b><?php
		$total_queries = $stats['query_should_cache'] + $stats['query_should_not_cache'];
		printf("%.2f%%", ($total_queries == 0) ? 0 : (100 / $total_queries) * $stats['cache_hit']);
		 ?></b></td>
		<td>(100 / (query_should_cache + query_should_not_cache)) * cache_hits</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td><b>Avg. speedup</b></td>
		<td style="white-space: nowrap;"><b><?php
		switch ($handler) {
			case 'default':
				$desc = "((query_aggr_run_time_cache_put + query_aggr_store_time_cache_put) / cache_put) / ((query_aggr_run_time_cache_hit + query_aggr_run_time_cache_hit) / cache_hit)";
				printf("%.2f x", (($stats['cache_put'] == 0) || ($stats['cache_hit'] == 0)) ? 0 :  (($stats['query_aggr_run_time_cache_put'] + $stats['query_aggr_store_time_cache_put']) / $stats['cache_put']) / (($stats['query_aggr_run_time_cache_hit'] + $stats['query_aggr_store_time_cache_hit']) / $stats['cache_hit']) );
				break;
			default:
				$desc = sprintf('n/a with handler "%s"', $handler);
				break;
		}

		 ?></b></td>
		<td><?php print $desc; ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td><b>Time saved</b></td>
		<td style="white-space: nowrap;"><b><?php
		switch ($handler) {
			default:
				$desc = '(query_aggr_run_time_cache_put + query_aggr_store_time_cache_put) * cache_hit / 1000 / 1000';
				$time = ($stats['query_aggr_run_time_cache_put'] + $stats['query_aggr_store_time_cache_put']) * $stats['cache_hit'];
				printf("%.2f s", $time / 1000 / 1000);
				break;
		}

		 ?></b></td>
		<td><?php print $desc; ?></td>
	</tr>
	<?php
	/*
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td><b>Avg. run time speedup</b></td>
		<td><b><?php
		switch ($handler) {
			case 'default':
				$desc = "(query_aggr_run_time_cache_put / cache_put) / (query_aggr_run_time_cache_hit / cache_hit)";
				printf("%.2f x", (($stats['cache_put'] == 0) || ($stats['cache_hit'] == 0)) ? 0 :
				($stats['query_aggr_run_time_cache_put'] / $stats['cache_put']) / ($stats['query_aggr_run_time_cache_hit'] / $stats['cache_hit'])
				);
				break;
			default:
				$desc = sprintf('n/a with handler "%s"', $handler);
				break;
		}

		 ?></b></td>
		<td><?php print $desc; ?></td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td style="white-space: nowrap;"><b>Avg. store time speedup</b></td>
		<td style="white-space: nowrap;"><b><?php
		switch ($handler) {
			case 'default':
				$desc = "(query_aggr_store_time_cache_put / cache_put) / (query_aggr_store_time_cache_hit / cache_hit)";
				printf("%.2f x", (($stats['cache_put'] == 0) || ($stats['cache_hit'] == 0)) ? 0 :
				($stats['query_aggr_store_time_cache_put'] / $stats['cache_put']) / ($stats['query_aggr_store_time_cache_hit'] / $stats['cache_hit'])
				);
				break;
			default:
				$desc = sprintf('n/a with handler "%s"', $handler);
				break;
		}

		 ?></b></td>
		<td><?php print $desc; ?></td>
	</tr>
	*/
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td style="white-space: nowrap;"><b>Saved traffic</b></td>
		<td style="white-space: nowrap;"><b><?php
		switch ($handler) {
			default:
				$desc = '(receive_bytes_replayed + send_bytes_replayed) / 1024 / 1024';
				$bytes = $stats['receive_bytes_replayed'] + $stats['send_bytes_replayed'];
				printf("%.2f MB", ($bytes == 0) ? 0 : $bytes / 1024 / 1024);
				break;
		}

		 ?></b></td>
		<td><?php print $desc; ?></td>
	</tr>
	<?php
	} // end if (empty($stats))
	?>
</table>

<?php
} // end func qc_print_main_core_stats_summary


/**
* Prints a cache core statistics details as a HTML table suitable for the main contents area
*
* Process scrop - core_stats()!
*/
function qc_print_main_core_stats_details() {
	$stats = mysqlnd_qc_get_core_stats();

	$color_idx = 0;
?>
<table class="box">
	<tr>
		<th>Statistic</th>
		<th>Value</th>
		<th>Notes</th>
	</tr>
	<?php
	if (empty($stats)) {
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td colspan="3">n/a - cache is empty</td>
	</tr>
	<?php
	} else {
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>cache_hit</td>
		<td><?php printf("%d", $stats["cache_hit"]); ?></td>
		<td>Cache hits. Possible reasons:
			<ol>
				<li>Query is cacheable , we have replayed a cached result set, store_result() has been used-.</li>
				<li>Query is cacheble, we have recorded the query result because we have not been sure if it had been cached but when adding to the cache we noticed someone else had it already added to the cache. In other words: another PHP request has been fast in adding it to the cache than we have been. Store_result() has been used.</li>
			</ol>
			Derived statistics:
			<ul>
				<li>???:<br /> <code>query_should_cache - cache_hit</code><br /> (<?php printf("%d + %d = %d", $stats['query_should_cache'], $stats['cache_hit'], $stats['query_should_cache'] - $stats['cache_hit']); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>cache_miss</td>
		<td><?php printf("%d", $stats["cache_miss"]); ?></td>
		<td>Cache misses. Possible reasons:
			<ol>
				<li>Query is cacheable and has generated a result set but at least one column has no table name (e.g. SELECT NOW(), SELECT 123, VERSION()).</li>
				<li>Query is cacheable  and has been added to the cache. Store_result() has been used.</li>
				<li>Query is cacheable  but store_result() has returned no data.</li>
				<li>Query is cacheable  but use_result() has been used.</li>
			</ol>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>cache_put</td>
		<td><?php printf("%d", $stats["cache_put"]); ?></td>
		<td>Entry added to the cache. Possible reasons:
			<ol>
				<li>Query is cacheable  and has been added to the cache. Store_result() has been used. Incremented together with cache_miss[2],</li>
			</ol>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_should_cache</td>
		<td><?php printf("%d", $stats["query_should_cache"]); ?></td>
		<td>Query may be cached. Possible reasons:
			<ol>
				<li>The storage handler has classified query as cacheable  based on the analysis of the query string (is_select() has returned true). Note: this does not mean that the query will be cached. It may have no table name, it may not generate a result set and so on.</li>
			</ol>
			Derived statistics:
			<ul>
				<li>Total number of queries inspected by the cache plugin:<br /> <code>query_should_cache + query_should_not_cache</code><br /> (<?php printf("%d + %d = %d", $stats['query_should_cache'], $stats['query_should_not_cache'], $stats['query_should_cache'] + $stats['query_should_not_cache']); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_should_not_cache</td>
		<td><?php printf("%d", $stats["query_should_not_cache"]); ?></td>
		<td>Query will not be cached. Possible reasons:
			<ol>
				<li>The storage handler has classified query as uncacheable  based on the analysis of the query string (is_select() has returned false).</li>
			</ol>
			Derived statistics:
			<ul>
				<li>Total number of queries inspected by the cache plugin:<br /> <code>query_should_cache + query_should_not_cache</code><br /> (<?php printf("%d + %d = %d", $stats['query_should_cache'], $stats['query_should_not_cache'], $stats['query_should_cache'] + $stats['query_should_not_cache']); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_not_cached</td>
		<td><?php printf("%d", $stats["query_not_cached"]); ?></td>
		<td>Query has not been cached. Possible reasons:
			<ol>
				<li>The storage handler has classified query as uncacheable  based on the analysis of the query string (is_select() has returned false).</li>
				<li>The storage handler has returned no hash key for the query (get_hash_key() has returned an empty string).</li>
			</ol>
			Derived statistics:
			<ul>
				<li>Total number of empty hash keys (query_not_cached[2]):<br /> <code>query_not_cached - query_should_not_cache</code><br /> (<?php printf("%d - %d = %d", $stats['query_not_cached'], $stats['query_should_not_cache'], $stats['query_not_cached'] - $stats['query_should_not_cache']); ?>). A value greater than zero may indicate problems with the cache storage. It is not know why the cache
				storage handler has failed to compute a hash key.
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_could_cache</td>
		<td><?php printf("%d", $stats["query_could_cache"]); ?></td>
		<td>Query is cacheable  (should be cached), may or may not be in the cache already, has been executed and metadata indicate a result set with at least one column.
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_found_in_cache</td>
		<td><?php printf("%d", $stats["query_found_in_cache"]); ?></td>
		<td>Query is cacheable  and has been found in the cache (find_query_in_cache() has returned true). We do not know yet, if
		we can replay the cached data which may give a true cache hit.
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_uncached_other</td>
		<td><?php printf("%d", $stats["query_uncached_other"]); ?></td>
		<td>Query is cacheable  (should be cached), may or may not be in the cache already, has been executed but either has generated no result set or some other error happened during query execution.
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_uncached_no_table</td>
		<td><?php printf("%d", $stats["query_uncached_no_table"]); ?></td>
		<td>Query would have had been added to the cache if <code>cache_no_table=0</code> had not prevented it.</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_uncached_use_result</td>
		<td><?php printf("%d", $stats["query_uncached_use_result"]); ?></td>
		<td>Query is cacheable  but has not been cached because use_result() has been used. Incremented together with cache_miss[4].
			Derived statistics:
			<ul>
				<li>Total number of queries that may be cached, if no errors and system tuned:<br /> <code>query_uncached_other + query_uncached_no_table + query_uncached_use_result</code><br /> (<?php printf("%d + %d + %d = %d",
					$stats['query_uncached_other'],
					$stats['query_uncached_no_table'],
					$stats['query_uncached_use_result'],
					$stats['query_uncached_other'] + $stats['query_uncached_no_table'] + $stats['query_uncached_use_result']); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_aggr_run_time_cache_hit</td>
		<td><?php printf("%d", $stats["query_aggr_run_time_cache_hit"]); ?></td>
		<td>Aggregated run time (ms) of all queries which are considered as a cache hit.
			Derived statistics:
			<ul>
				<li>Average run time (ms) of a cached query:<br /> <code>query_aggr_run_time_cache_hit / cache_hit</code><br /> (<?php printf("%d - %d = %d",
					$stats['query_aggr_run_time_cache_hit'],
					$stats['cache_hit'],
					($stats['cache_hit'] == 0) ? 0 : ($stats['query_aggr_run_time_cache_hit'] / $stats['cache_hit'])); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_aggr_run_time_cache_put</td>
		<td><?php printf("%d", $stats["query_aggr_run_time_cache_put"]); ?></td>
		<td>Aggregated run time (ms) of all queries which are considered as a cache put.
			Derived statistics:
			<ul>
				<li>Average run time improvement because of caching:<br /> <code>((query_aggr_run_time_cache_hit + query_aggre_run_time_cache_put) / (cache_hit + cache_put))</code><br /> (<?php printf("(%d + %d) / (%d + %d) = %d",
					$stats['query_aggr_run_time_cache_hit'],
					$stats['query_aggr_run_time_cache_put'],
					$stats['cache_hit'],
					$stats['cache_put'],
					(($stats['cache_hit'] + $stats['cache_put']) == 0) ? 0 : (($stats['query_aggr_run_time_cache_hit'] + $stats['query_aggr_run_time_cache_put']) / ($stats['cache_hit'] + $stats['cache_put']))); ?>)
				</li>
				<li>Average run time (ms) of a query which has been added to the cache:<br /> <code>query_aggr_run_time_cache_put / cache_put</code><br /> (<?php printf("%d - %d = %d",
					$stats['query_aggr_run_time_cache_put'],
					$stats['cache_put'],
					($stats['cache_put'] == 0) ? 0 : ($stats['query_aggr_run_time_cache_put'] / $stats['cache_put'])); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_aggr_run_time_total</td>
		<td><?php printf("%d", $stats["query_aggr_run_time_total"]); ?></td>
		<td>Aggregated run time (ms) of all queries run by the query cache.
			Derived statistics:
			<ul>
				<li>Aggregated run time (ms) of all uncached queries (errors and cache misses) queries:<br /> <code>query_aggr_run_time_total - query_aggr_run_time_cache_hit</code><br /> (<?php printf("%d - %d = %d",
					$stats['query_aggr_run_time_total'],
					$stats['query_aggr_run_time_cache_hit'],
					$stats['query_aggr_run_time_total'] - $stats['query_aggr_run_time_cache_hit']); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_aggr_store_time_cache_hit</td>
		<td><?php printf("%d", $stats["query_aggr_store_time_cache_hit"]); ?></td>
		<td>Aggregated store time (ms) of all queries which are considered as a cache hit.
			Derived statistics:
			<ul>
				<li>Average store time (ms) of a cached query:<br /> <code>query_aggr_store_time_cache_hit / cache_hit</code><br /> (<?php printf("%d / %d = %d",
					$stats['query_aggr_store_time_cache_hit'],
					$stats['cache_hit'],
					($stats['cache_hit'] == 0) ? 0 : ($stats['query_aggr_store_time_cache_hit'] / $stats['cache_hit'])); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_aggr_run_time_cache_put</td>
		<td><?php printf("%d", $stats["query_aggr_store_time_cache_put"]); ?></td>
		<td>Aggregated store time (ms) of all queries which are considered as a cache put.
			Derived statistics:
			<ul>
				<li>Average store time improvement because of caching:<br /> <code>((query_aggr_store_time_cache_hit + query_aggre_store_time_cache_put) / (cache_hit + cache_put))</code><br /> (<?php printf("(%d + %d) / (%d + %d) = %d",
					$stats['query_aggr_store_time_cache_hit'],
					$stats['query_aggr_store_time_cache_put'],
					$stats['cache_hit'],
					$stats['cache_put'],
					(($stats['cache_hit'] + $stats['cache_put']) == 0) ? 0 : (($stats['query_aggr_store_time_cache_hit'] + $stats['query_aggr_store_time_cache_put']) / ($stats['cache_hit'] + $stats['cache_put']))); ?>)
				</li>
				<li>Average run time (ms) of a query which has been added to the cache:<br /> <code>query_aggr_run_time_cache_put / cache_put</code><br /> (<?php printf("%d - %d = %d",
					$stats['query_aggr_store_time_cache_put'],
					$stats['cache_put'],
					($stats['cache_put'] == 0) ? 0 : ($stats['query_aggr_store_time_cache_put'] / $stats['cache_put'])); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>query_aggr_store_time_total</td>
		<td><?php printf("%d", $stats["query_aggr_store_time_total"]); ?></td>
		<td>Aggregated store time (ms) of all queries run by the query cache.
			Derived statistics:
			<ul>
				<li>Aggregated store time (ms) of all uncached queries (errors and cache misses) queries:<br /> <code>query_aggr_store_time_total - query_aggr_store_time_cache_hit</code><br /> (<?php printf("%d - %d = %d",
					$stats['query_aggr_store_time_total'],
					$stats['query_aggr_store_time_cache_hit'],
					$stats['query_aggr_store_time_total'] - $stats['query_aggr_store_time_cache_hit']); ?>)
				</li>
			</ul>
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>receive_bytes_recorded</td>
		<td><?php printf("%d", $stats["receive_bytes_recorded"]); ?></td>
		<td>Total number of bytes received from MySQL and recorded for replay in case of a potential cache hit. The recorded payload
		may or may not be added to the cache for replay. It will not be added to the cache for those queries which are not cached because of <code>query_uncached_other</code>, <code>query_uncached_no_table</code>, <code>query_uncached_use_result</code>. The received data consists of result sets, meta data and status codes.
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>receive_bytes_replayed</td>
		<td><?php printf("%d", $stats["receive_bytes_replayed"]); ?></td>
		<td>Total number of bytes of incoming traffic (from MySQL to PHP) replayed from the cache. In other words: traffic you saved because of caching.
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>send_bytes_recorded</td>
		<td><?php printf("%d", $stats["send_bytes_recorded"]); ?></td>
		<td>Total number of bytes send from PHP to MySQL and recorded for replay in case of a potential cache hit. The recorded payload
		may or may not be added to the cache for replay. It will not be added to the cache for those queries which are not cached because of <code>query_uncached_other</code>, <code>query_uncached_no_table</code>, <code>query_uncached_use_result</code>. The send data consists of query strings.
		</td>
	</tr>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td>send_bytes_replayed</td>
		<td><?php printf("%d", $stats["send_bytes_replayed"]); ?></td>
		<td>Total number of bytes of outgoing traffic (from PHP to MySQL) replayed from the cache. In other words: traffic you saved because of caching.
		Derived statistics:
			<ul>
				<li>Total network traffic saving in MB:<br /> <code>(receive_bytes_replayed + send_bytes_replayed) / 1024 / 1024</code><br /> (<?php printf("(%d + %d) / 1024 / 1024 = %.2f",
					$stats['receive_bytes_replayed'],
					$stats['send_bytes_replayed'],
					($stats['receive_bytes_replayed'] + $stats['send_bytes_replayed']) / 1024 / 1024); ?>)
				</li>
			</ul>
		</td>
	</tr>
</table>
	<?php
	} // if (empty($info['data'])) {
} // end func qc_print_main_core_stats_details

/**
* Prints auto append (MySQL storage) details as a HTML table suitable for the main contents area
*
*/
function qc_print_main_auto_append_core_stats_details() {

	$color_idx = 0;

	$link = qc_get_connection();
	if (!$res = $link->query("
	 	SELECT
			r.pid AS _pid,
			MIN(r.created) AS _from,
			MAX(r.created) AS _to,
			MAX(r.request_counter) - MIN(r.request_counter) + 1 AS _requests,
			AVG(c.derived_cache_hit_ratio) AS _hit_ratio,
			AVG(c.derived_cache_efficiency) AS _efficiency,
			SUM(c.derived_time_saved) AS _time_saved,
			SUM(c.derived_traffic_saved) AS _traffic_saved
		FROM
			qc_auto_append_statistics_record AS r
		JOIN
			qc_auto_append_core_statistics AS c
		ON
			r.id = c.fk_statistics_record_id
		GROUP BY
			r.pid_process_hash
		ORDER BY
			r.id DESC
		LIMIT 100
	"))
		throw new Exception("[%d] %s\n", $link->errno, $link->error);

?>
<table class="box">
	<tr>
		<th>PID</th>
		<th>Number of requests</th>
		<th>First request</th>
		<th>Last request</th>
		<th>Cache efficiency %</th>
		<th>Time saved (s)</th>
		<th>Traffic saved (MB)</th>
	</tr>
	<?php
	$efficiency = 0;
	$requests = 0;
	$time_saved = 0;
	$traffic_saved = 0;
	while ($row = $res->fetch_assoc()) {
		$efficiency += $row['_efficiency'];
		$requests += $row['_requests'];
		$time_saved += $row['_time_saved'];
		$traffic_saved += $row['_traffic_saved'];
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td align="right"><?php print $row['_pid']; ?></td>
		<td align="right"><?php print $row['_requests']; ?></td>
		<td align="right"><?php print qc_db_date_format($row['_from']); ?></td>
		<td align="right"><?php print qc_db_date_format($row['_to']); ?></td>
		<td align="right"><?php printf("%.2f%%", $row['_efficiency']); ?></td>
		<td align="right"><?php printf("%.2fs", $row['_time_saved']); ?></td>
		<td align="right"><?php printf("%.2fs", $row['_traffic_saved']); ?></td>
	</tr>
	<?php
	} // while ($row = $res->fetch_assoc())
	$num_rows = $res->num_rows;
	$res->close();
	if ($num_rows > 0) {
	?>
	<tr bgcolor="<?php print ($color_idx++ % 2 == 0) ? QCM_TD_LIST_COLOR0 : QCM_TD_LIST_COLOR1; ?>">
		<td align="right">Summary</td>
		<td align="right"><?php print $requests; ?></td>
		<td align="right">n/a</td>
		<td align="right">n/a</td>
		<td align="right"><?php printf("%.2f%%", $efficiency / $num_rows); ?></td>
		<td align="right"><?php printf("%.2fs", $time_saved / $num_rows); ?></td>
		<td align="right"><?php printf("%.2fs", $traffic_saved / $num_rows); ?></td>
	</tr>
</table>
	<?php
	}
} // end func qc_print_main_auto_append_details
?>
