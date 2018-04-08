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
require_once("mysqlnd_qc_monitor_html_header_inc.php");
?>
<div class="main">
	<h1>Core cache statistics</h1>
	<p>
	The data shown below is based on the API call <code>mysqlnd_qc_get_core_statistics()</code>.
	Core statistics are collected by the core of the query cache plugin. The core stores
	them in a hash table (process memory) accordingly the scope of the data shown is that
	of the current process. Other processes may give you entirely different results!
	</p>
	<p>
	<b>Results are only valid for the current process.</b>
	</p>
	<h2>Summary</h2>
	<p>
		<?php qc_print_main_core_stats_summary(); ?>
	</p>
	<h2>Details</h2>
	<p>
		<?php qc_print_main_core_stats_details(); ?>
	</p>
	?>
</div>
<div class="box">
	<h3>Versions</h3>
	<?php qc_print_box_version_list(); ?>
</div>
<div class="box">
	<h3>Active Handler</h3>
	<?php qc_print_box_active_handler_summary(); ?>
</div>
<div class="box">
	<h3>PHP configuration (php.ini)</h3>
	<?php qc_print_box_ini_settings_summary(); ?>
</div>
<div class="box">
	<h3>Constants</h3>
	<?php qc_print_box_constants(); ?>
</div>
<?php
require_once("mysqlnd_qc_monitor_html_footer_inc.php");
?>