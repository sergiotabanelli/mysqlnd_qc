
PHP_ARG_ENABLE(mysqlnd_qc, whether to enable mysqlnd_qc support,
[  --enable-mysqlnd-qc           Enable mysqlnd_qc support])
PHP_ARG_ENABLE(mysqlnd_qc_apc, whether to enable mysqlnd_qc APC support,
[  --enable-mysqlnd-qc-apc       Enable mysqlnd_qc APC support],no,no)
PHP_ARG_ENABLE(mysqlnd_qc_memcache, whether to enable mysqlnd_qc Memcache support,
[  --enable-mysqlnd-qc-memcache       Enable mysqlnd_qc Memcache support],no,no)
PHP_ARG_WITH(libmemcached-dir,  for libmemcached,
[  --with-libmemcached-dir[=DIR]   Set the path to libmemcached install prefix.], yes)
PHP_ARG_ENABLE(mysqlnd_qc_sqlite, whether to enable mysqlnd_qc SQLite support,
[  --enable-mysqlnd-qc-sqlite       Enable mysqlnd_qc SQLite support],no,no)
PHP_ARG_WITH(sqlite-dir,  for sqlite,
[  --with-sqlite-dir[=DIR]   Set the path to SQLite3 install prefix.], yes)

if test "$PHP_MYSQLND_QC" && test "$PHP_MYSQLND_QC" != "no"; then
  PHP_SUBST(MYSQLND_QC_SHARED_LIBADD)

  mysqlnd_qc_sources="php_mysqlnd_qc.c mysqlnd_qc.c mysqlnd_qc_ps.c mysqlnd_qc_logs.c mysqlnd_qc_zval_util.c mysqlnd_qc_user_handler.c mysqlnd_qc_std_handler.c mysqlnd_qc_classes.c mysqlnd_qc_object_handler.c mysqlnd_qc_nop_handler.c mysqlnd_qc_tokenize.c"

  PHP_ADD_EXTENSION_DEP(mysqlnd_qc, mysqlnd)

  if test "$PHP_MYSQLND_QC_MEMCACHE" != "no"; then
    if test "$PHP_LIBMEMCACHED_DIR" != "no" && test "$PHP_LIBMEMCACHED_DIR" != "yes"; then
      if test -r "$PHP_LIBMEMCACHED_DIR/include/libmemcached/memcached.h"; then
        PHP_LIBMEMCACHED_DIR="$PHP_LIBMEMCACHED_DIR"
      else
        AC_MSG_ERROR([Can't find libmemcached headers under "$PHP_LIBMEMCACHED_DIR"])
      fi
    else
      PHP_LIBMEMCACHED_DIR="no"
      for i in /usr /usr/local; do
        if test -r "$i/include/libmemcached/memcached.h"; then
          PHP_LIBMEMCACHED_DIR=$i
          break
        fi
      done
    fi

    AC_MSG_CHECKING([for libmemcached location])
    if test "$PHP_LIBMEMCACHED_DIR" = "no"; then
      AC_MSG_ERROR([memcached support requires libmemcached. Use --with-libmemcached-dir=<DIR> to specify the prefix where libmemcached headers and library are located])
    else
      AC_MSG_RESULT([$PHP_LIBMEMCACHED_DIR])
      PHP_LIBMEMCACHED_INCDIR="$PHP_LIBMEMCACHED_DIR/include"
      PHP_ADD_INCLUDE($PHP_LIBMEMCACHED_INCDIR)
      PHP_CHECK_LIBRARY(memcached,memcached_free,
      [
         PHP_ADD_LIBRARY_WITH_PATH(memcached, $PHP_LIBMEMCACHED_DIR/$PHP_LIBDIR, MYSQLND_QC_SHARED_LIBADD)
      ],[
        AC_MSG_ERROR([wrong memcached lib not found in $PHP_LIBMEMCACHED_DIR/$PHP_LIBDIR])
      ],[
        -L$PHP_LIBMEMCACHED_DIR/$PHP_LIBDIR
      ])

      AC_DEFINE([MYSQLND_QC_HAVE_MEMCACHE], 1, [Whether memcache support is enabled])
      PHP_CHECK_LIBRARY(memcached, memcache_exists, [AC_DEFINE(HAVE_MEMCACHE_EXISTS, 1, [ ])], [ ], [ ])
      mysqlnd_qc_sources="$mysqlnd_qc_sources mysqlnd_qc_memcache_handler.c"
    fi

  fi

  if test "$PHP_MYSQLND_QC_APC" != "no"; then
    PHP_ADD_EXTENSION_DEP(mysqlnd_qc, apc)
    if test "$PHP_APC" = "no" || test "x$PHP_APC" = "x"; then
      AC_MSG_ERROR([APC is onlysupported if both APC and MySQL Query Cache are compiled statically])
    fi
    AC_DEFINE([MYSQLND_QC_HAVE_APC], 1, [Whether APC is enabled])
    mysqlnd_qc_sources="$mysqlnd_qc_sources mysqlnd_qc_apc_handler.c"
  fi

  if test "$PHP_MYSQLND_QC_SQLITE" != "no"; then
    AC_MSG_CHECKING([for SQLite 3 header location])
    if test "$PHP_SQLITE_DIR" = "no" || test "$PHP_SQLITE_DIR" = "yes" || test "x$PHP_SQLITE_DIR" = "x"; then
      AC_MSG_RESULT([using bundled])
      if test -r "$abs_srcdir/ext/sqlite3/libsqlite/sqlite3.h"; then
        MYSQLND_QC_SQLITE_DIR="$abs_srcdir/ext/sqlite3/libsqlite"
      else
        AC_MSG_ERROR([SQLite3 header not found, please use --with-sqlite-dir=DIR])
      fi
    else
      AC_MSG_RESULT([$PHP_SQLITE_DIR])
      if test -r "$PHP_SQLITE_DIR/include/sqlite3.h"; then
        MYSQLND_QC_SQLITE_DIR="$PHP_SQLITE_DIR/include"
        MYSQLND_QC_SQLITE_LIB_DIR="$PHP_SQLITE_DIR/$PHP_LIBDIR"

        PHP_CHECK_LIBRARY(sqlite3,sqlite3_open,
        [
           PHP_ADD_LIBRARY_WITH_PATH(sqlite3, $MYSQLND_QC_SQLITE_LIB_DIR, MYSQLND_QC_SHARED_LIBADD)
        ],[
          AC_MSG_ERROR([wrong SQLite3 lib not found])
        ],[
          -L$MYSQLND_QC_SQLITE_LIB_DIR/$PHP_LIBDIR -lm
        ])
      else
        AC_MSG_ERROR([SQLite3 header not found in $PHP_SQLITE_DIR.])
      fi
    fi

    PHP_ADD_INCLUDE($MYSQLND_QC_SQLITE_DIR)


    AC_DEFINE([MYSQLND_QC_HAVE_SQLITE], 1, [Whether SQLite is enabled])
    mysqlnd_qc_sources="$mysqlnd_qc_sources mysqlnd_qc_sqlite_handler.c"
    PHP_ADD_EXTENSION_DEP(mysqlnd_qc, sqlite3)
  fi

  PHP_NEW_EXTENSION(mysqlnd_qc, $mysqlnd_qc_sources, $ext_shared)
  PHP_INSTALL_HEADERS([ext/mysqlnd_qc/])
else

  if test "$PHP_MYSQLND_QC_MEMCACHE" != "no"; then
    AC_MSG_ERROR([You have enabled mysqlnd_qc Memcache support but you did not enable mysqlnd_qc support. Please check your configuration!])
  fi

  if test "$PHP_MYSQLND_QC_APC" != "no"; then
    AC_MSG_ERROR([You have enabled mysqlnd_qc APC support but you did not enable mysqlnd_qc support. Please check your configuration!])
  fi

  if test "$PHP_MYSQLND_QC_SQLITE" != "no"; then
    AC_MSG_ERROR([You have enabled mysqlnd_qc SQLite support but you did not enable mysqlnd_qc support. Please check your configuration!])
  fi

fi
