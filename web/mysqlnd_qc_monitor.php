<?php
/**
* Basic cache monitor
*
*/
require_once("mysqlnd_qc_monitor_core_functions_inc.php");
require_once("mysqlnd_qc_monitor_html_header_inc.php");
?>
<div class="main">
	<h1>Most basic query cache monitor</h1>
	<p>
		<b>First, this is not a production level monitor. This does not aim to be on production level!</b>
	</p>
	<p>
		It is a collection of scripts that turned out to be handy during the
		development of the query cache. Do not take it as more than that.
		It is useful for quick sanity checks. And it demonstrates what
		you can do with the data from those functions that return statistics
		and related information:
		<ul>
			<li><code>mysqlnd_qc_get_cache_info()</code></li>
			<li><code>mysqlnd_qc_get_core_stats()</code></li>
		</ul>
	</p>
	<h2>Pitfall: scope of data</h2>
	<p>
		Depending on the function that deliveres the data, the storage
		handler in use and your deployment model, data may be available
		on different basis:
		<ul>
			<li>per process (one or multiple web requests)</li>
			<li>per machine (one or multiple processes)</li>
			<li>global (one or multiple machines)</li>
		</ul>
	</p>
	<p>
		In most cases a note will be given to help you interpreting the
		figures you see. However, this is a hacker tool, not meant for
		end-users.
	</p>
	<h2>Tell me more...</h2>
	<p>
		<ul>
			<li><a href="mysqlnd_qc_monitor_core_statistics.php">Core Statistics</a></li>
			<li><a href="mysqlnd_qc_monitor_handler_statistics.php">Cache Contents</a></li>
			<li><a href="mysqlnd_qc_monitor_auto_append_mysql.php">Auto append summary (MySQL storage)</a></li>
		</ul>
	</p>
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
<div class="box">
	<h3>Summary (handler dependent!)</h3>
	<?php qc_print_box_cache_handler_summary(); ?>
</div>


<?php
require_once("mysqlnd_qc_monitor_html_footer_inc.php");
?>