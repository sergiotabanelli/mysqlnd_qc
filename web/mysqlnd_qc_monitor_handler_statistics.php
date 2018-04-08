<?php
/**
* Handler statistics (primarily cache contents)
*
*/
require_once("mysqlnd_qc_monitor_core_functions_inc.php");

if (isset($_GET["handler_stats_csv"])) {
	/* Stream  CSV file */
	qc_stream_cache_contents(-1);
	exit(0);
}
require_once("mysqlnd_qc_monitor_html_header_inc.php");
?>
<div class="main">
	<h1>Cache contents and storage handler statistics</h1>
	<p>
		The below figures are based	on data provided by the storage handler.
		Some handler do not provide statistics.	Some handler provide per-process statistics (e.g. Default).
		Others do provide per-machine (multiple processes) statistics (e.g. APC). Keep this in mind when interpreting the figures.
	</p>
	<h2>Download (CSV)</h2>
	<p>
		<a href="<?php print $_SERVER['SCRIPT_NAME']; ?>?handler_stats_csv=1">Download cache contents (CSV)</a>
	</p>
	<h2>Cache contents</h2>
	<p>
		Below is a list of up to 100 cached queries ordered by the amount
		of time saved because of caching the queries. Highest time savings
		come first.
	</p>
	<p>
		<?php qc_print_main_cache_handler_details(100, QCM_SORT_TIME_SAVED); ?>
	</p>
</div>
<div class="box">
	<h3>Summary (handler dependent!)</h3>
	<?php qc_print_box_cache_handler_summary(); ?>
</div>
<div class="box">
	<h3>Active Handler</h3>
	<?php qc_print_box_active_handler_summary(); ?>
</div>
<?php
require_once("mysqlnd_qc_monitor_html_footer_inc.php");
?>