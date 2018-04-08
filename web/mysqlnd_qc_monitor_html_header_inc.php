<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
	<title><?php print QCM_HTML_TITLE; ?></title>
	<style><!--
		body { margin:0; padding:0; }
		body,td,th,input,submit { font-family:arial,helvetica,sans-serif; font-size: 0.8em; }
		p { font-size: 100% }
		td { vertical-align:top; }
		th { color:<?php print QCM_CSS_DIV_TEXT_LIGHT; ?>; background-color:<?php print QCM_CSS_DIV_BACKGROUND; ?>; }
		h1 { font-size: 150%; padding: 0.1em; color: <?php print QCM_HEADLINE_COLOR; ?>;}
		h2 { font-size: 120%; padding: 0.1em; color: <?php print QCM_HEADLINE_COLOR; ?>; }
		h3 { font-size: 100%; font-weight:bold; padding: 0.1em;  color: <?php print QCM_HEADLINE_COLOR; ?>;}
		a { color:black; font-weight:none; text-decoration:none; }
		a:hover { text-decoration:underline; }
		div.content { width: 1200px; padding:1em 1em 1em 1em; }
		div.head { padding: 0.7em; background-color:<?php print QCM_CSS_DIV_BACKGROUND; ?>; color:<?php print QCM_CSS_DIV_TEXT_LIGHT; ?>; width: 100% }
		div.box { width:350px; background-color:<?php print QCM_CSS_DIV_BACKGROUND; ?>; padding: 0; margin: 0.5em; border: 1px solid <?php print QCM_CSS_DIV_BORDER; ?> }
		div.main { width: 800px; float: right; padding: 2px; margin: 0.5em; border: 1px solid <?php print QCM_CSS_DIV_BORDER; ?> }
		div.navigation_bar { width: 100%; background-color:<?php print QCM_CSS_DIV_BACKGROUND; ?>; color:<?php print QCM_CSS_DIV_TEXT_LIGHT; ?>; padding: 0.7em; margin-top: 0.1em; border: 1px solid <?php print QCM_CSS_DIV_BORDER; ?>; font-size: 100%; font-weight:bold; }
		table.box { width:100%; margin:0; padding:0}
	//--></style>
</head>
<body>
	<div class="head">
		<h1><?php print QCM_HTML_TITLE; ?></h1>
	</div>
	<div class="navigation_bar">
	<a href="mysqlnd_qc_monitor.php">Start</a>
	--- <a href="mysqlnd_qc_monitor_core_statistics.php">Core Statistics</a>
	--- <a href="mysqlnd_qc_monitor_handler_statistics.php">Cache Contents</a>
	--- <a href="mysqlnd_qc_monitor_auto_append_mysql.php">Auto append summary (MySQL storage)</a>
	</div>
	<div class="content"><!-- end of <?php print __FILE__; ?> -->