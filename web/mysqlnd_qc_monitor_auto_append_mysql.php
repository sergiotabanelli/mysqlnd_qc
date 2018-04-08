<?php
/**
* Basic cache web monitor
*
* TODO:
*
* - support other cache handler but APC
* - display more cache details
* - allow sorting of list of cached entries
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
	<h1>Auto append summary</h1>
	<p>
		Most statistics are provided on a per-process basis. Using the
		PHP <code>auto_append</code> feature and script
		<code>auto_append_persist_qc_stats.php</code> from the <code>web/</code>
		directory of the query cache you can, however, record all
		statistics at the end of a web request and write their values
		into the MySQL database to overcome scope/life-time limitations.
	</p>
	<p>
		<b>Experimental, work in progress, time consuming and slow, ...</b>
	</p>
	<p>
		<?php qc_print_main_auto_append_core_stats_details(); ?>
	</p>
</div>
<div class="box">
	<h3>Auto append summary</h3>
	<?php qc_print_box_append_summary(); ?>
</div>
<div class="box">
	<h3>Versions</h3>
	<?php qc_print_box_version_list(); ?>
</div>
<div class="box">
	<h3>Active Handler</h3>
	<?php qc_print_box_active_handler_summary(); ?>
</div>
<?php
require_once("mysqlnd_qc_monitor_html_footer_inc.php");
?>