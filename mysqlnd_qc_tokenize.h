/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

/* $Id: $ */

#ifndef MYSQLND_QC_TOKENIZE_H
#define MYSQLND_QC_TOKENIZE_H

enum qc_tokens
{
 QC_TOKEN_ABORT                     /* INTERNAL (used in lex) */
,QC_TOKEN_ACCESSIBLE
,QC_TOKEN_ACTION                        /* SQL-2003-N */
,QC_TOKEN_ADD                           /* SQL-2003-R */
,QC_TOKEN_ADDDATE                   /* MYSQL-FUNC */
,QC_TOKEN_AFTER                     /* SQL-2003-N */
,QC_TOKEN_AGAINST
,QC_TOKEN_AGGREGATE
,QC_TOKEN_ALGORITHM
,QC_TOKEN_ALL                           /* SQL-2003-R */
,QC_TOKEN_ALTER                         /* SQL-2003-R */
,QC_TOKEN_ANALYZE
,QC_TOKEN_AND_AND                   /* OPERATOR */
,QC_TOKEN_AND                       /* SQL-2003-R */
,QC_TOKEN_ANY                       /* SQL-2003-R */
,QC_TOKEN_AS                            /* SQL-2003-R */
,QC_TOKEN_ASC                           /* SQL-2003-N */
,QC_TOKEN_ASCII                     /* MYSQL-FUNC */
,QC_TOKEN_ASENSITIVE                /* FUTURE-USE */
,QC_TOKEN_AT                        /* SQL-2003-R */
,QC_TOKEN_AUTHORS
,QC_TOKEN_AUTOEXTEND_SIZE
,QC_TOKEN_AUTO_INC
,QC_TOKEN_AVG_ROW_LENGTH
,QC_TOKEN_AVG                       /* SQL-2003-N */
,QC_TOKEN_BACKUP
,QC_TOKEN_BEFORE                    /* SQL-2003-N */
,QC_TOKEN_BEGIN                     /* SQL-2003-R */
,QC_TOKEN_BETWEEN                   /* SQL-2003-R */
,QC_TOKEN_BIGINT                        /* SQL-2003-R */
,QC_TOKEN_BINARY                        /* SQL-2003-R */
,QC_TOKEN_BINLOG
,QC_TOKEN_BIN_NUM
,QC_TOKEN_BIT_AND                       /* MYSQL-FUNC */
,QC_TOKEN_BIT_OR                        /* MYSQL-FUNC */
,QC_TOKEN_BIT                       /* MYSQL-FUNC */
,QC_TOKEN_BIT_XOR                       /* MYSQL-FUNC */
,QC_TOKEN_BLOB                      /* SQL-2003-R */
,QC_TOKEN_BLOCK
,QC_TOKEN_BOOLEAN                   /* SQL-2003-R */
,QC_TOKEN_BOOL
,QC_TOKEN_BOTH                          /* SQL-2003-R */
,QC_TOKEN_BTREE
,QC_TOKEN_BY                            /* SQL-2003-R */
,QC_TOKEN_BYTE
,QC_TOKEN_CACHE
,QC_TOKEN_CALL                      /* SQL-2003-R */
,QC_TOKEN_CASCADE                       /* SQL-2003-N */
,QC_TOKEN_CASCADED                      /* SQL-2003-R */
,QC_TOKEN_CASE                      /* SQL-2003-R */
,QC_TOKEN_CAST                      /* SQL-2003-R */
,QC_TOKEN_CATALOG_NAME              /* SQL-2003-N */
,QC_TOKEN_CHAIN                     /* SQL-2003-N */
,QC_TOKEN_CHANGE
,QC_TOKEN_CHANGED
,QC_TOKEN_CHARSET
,QC_TOKEN_CHAR                      /* SQL-2003-R */
,QC_TOKEN_CHECKSUM
,QC_TOKEN_CHECK                     /* SQL-2003-R */
,QC_TOKEN_CIPHER
,QC_TOKEN_CLASS_ORIGIN              /* SQL-2003-N */
,QC_TOKEN_CLIENT
,QC_TOKEN_CLOSE                     /* SQL-2003-R */
,QC_TOKEN_COALESCE                      /* SQL-2003-N */
,QC_TOKEN_CODE
,QC_TOKEN_COLLATE                   /* SQL-2003-R */
,QC_TOKEN_COLLATION                 /* SQL-2003-N */
,QC_TOKEN_COLUMNS
,QC_TOKEN_COLUMN                    /* SQL-2003-R */
,QC_TOKEN_COLUMN_NAME               /* SQL-2003-N */
,QC_TOKEN_COMMENT
,QC_TOKEN_COMMITTED                 /* SQL-2003-N */
,QC_TOKEN_COMMIT                    /* SQL-2003-R */
,QC_TOKEN_COMPACT
,QC_TOKEN_COMPLETION
,QC_TOKEN_COMPRESSED
,QC_TOKEN_CONCURRENT
,QC_TOKEN_CONDITION                 /* SQL-2003-R, SQL-2008-R */
,QC_TOKEN_CONNECTION
,QC_TOKEN_CONSISTENT
,QC_TOKEN_CONSTRAINT                    /* SQL-2003-R */
,QC_TOKEN_CONSTRAINT_CATALOG        /* SQL-2003-N */
,QC_TOKEN_CONSTRAINT_NAME           /* SQL-2003-N */
,QC_TOKEN_CONSTRAINT_SCHEMA         /* SQL-2003-N */
,QC_TOKEN_CONTAINS                  /* SQL-2003-N */
,QC_TOKEN_CONTEXT
,QC_TOKEN_CONTINUE                  /* SQL-2003-R */
,QC_TOKEN_CONTRIBUTORS
,QC_TOKEN_CONVERT                   /* SQL-2003-N */
,QC_TOKEN_COUNT                     /* SQL-2003-N */
,QC_TOKEN_CPU
,QC_TOKEN_CREATE                        /* SQL-2003-R */
,QC_TOKEN_CROSS                         /* SQL-2003-R */
,QC_TOKEN_CUBE                      /* SQL-2003-R */
,QC_TOKEN_CURDATE                       /* MYSQL-FUNC */
,QC_TOKEN_CURRENT_USER                  /* SQL-2003-R */
,QC_TOKEN_CURSOR                    /* SQL-2003-R */
,QC_TOKEN_CURSOR_NAME               /* SQL-2003-N */
,QC_TOKEN_CURTIME                       /* MYSQL-FUNC */
,QC_TOKEN_DATABASE
,QC_TOKEN_DATABASES
,QC_TOKEN_DATAFILE
,QC_TOKEN_DATA                      /* SQL-2003-N */
,QC_TOKEN_DATETIME
,QC_TOKEN_DATE_ADD_INTERVAL             /* MYSQL-FUNC */
,QC_TOKEN_DATE_SUB_INTERVAL             /* MYSQL-FUNC */
,QC_TOKEN_DATE                      /* SQL-2003-R */
,QC_TOKEN_DAY_HOUR
,QC_TOKEN_DAY_MICROSECOND
,QC_TOKEN_DAY_MINUTE
,QC_TOKEN_DAY_SECOND
,QC_TOKEN_DAY                       /* SQL-2003-R */
,QC_TOKEN_DEALLOCATE                /* SQL-2003-R */
,QC_TOKEN_DECIMAL_NUM
,QC_TOKEN_DECIMAL                   /* SQL-2003-R */
,QC_TOKEN_DECLARE                   /* SQL-2003-R */
,QC_TOKEN_DEFAULT                       /* SQL-2003-R */
,QC_TOKEN_DEFINER
,QC_TOKEN_DELAYED
,QC_TOKEN_DELAY_KEY_WRITE
,QC_TOKEN_DELETE                    /* SQL-2003-R */
,QC_TOKEN_DESC                          /* SQL-2003-N */
,QC_TOKEN_DESCRIBE                      /* SQL-2003-R */
,QC_TOKEN_DES_KEY_FILE
,QC_TOKEN_DETERMINISTIC             /* SQL-2003-R */
,QC_TOKEN_DIRECTORY
,QC_TOKEN_DISABLE
,QC_TOKEN_DISCARD
,QC_TOKEN_DISK
,QC_TOKEN_DISTINCT                      /* SQL-2003-R */
,QC_TOKEN_DIV
,QC_TOKEN_DOUBLE                    /* SQL-2003-R */
,QC_TOKEN_DO
,QC_TOKEN_DROP                          /* SQL-2003-R */
,QC_TOKEN_DUAL
,QC_TOKEN_DUMPFILE
,QC_TOKEN_DUPLICATE
,QC_TOKEN_DYNAMIC                   /* SQL-2003-R */
,QC_TOKEN_EACH                      /* SQL-2003-R */
,QC_TOKEN_ELSE                          /* SQL-2003-R */
,QC_TOKEN_ELSEIF
,QC_TOKEN_ENABLE
,QC_TOKEN_ENCLOSED
,QC_TOKEN_END                           /* SQL-2003-R */
,QC_TOKEN_ENDS
,QC_TOKEN_END_OF_INPUT                  /* INTERNAL */
,QC_TOKEN_ENGINES
,QC_TOKEN_ENGINE
,QC_TOKEN_ENUM
,QC_TOKEN_EQ                            /* OPERATOR */
,QC_TOKEN_EQUAL                     /* OPERATOR */
,QC_TOKEN_ERRORS
,QC_TOKEN_ESCAPED
,QC_TOKEN_ESCAPE                    /* SQL-2003-R */
,QC_TOKEN_EVENTS
,QC_TOKEN_EVENT
,QC_TOKEN_EVERY                     /* SQL-2003-N */
,QC_TOKEN_EXECUTE                   /* SQL-2003-R */
,QC_TOKEN_EXISTS                        /* SQL-2003-R */
,QC_TOKEN_EXIT
,QC_TOKEN_EXPANSION
,QC_TOKEN_EXTENDED
,QC_TOKEN_EXTENT_SIZE
,QC_TOKEN_EXTRACT                   /* SQL-2003-N */
,QC_TOKEN_FALSE                     /* SQL-2003-R */
,QC_TOKEN_FAST
,QC_TOKEN_FAULTS
,QC_TOKEN_FETCH                     /* SQL-2003-R */
,QC_TOKEN_FILE
,QC_TOKEN_FIRST                     /* SQL-2003-N */
,QC_TOKEN_FIXED
,QC_TOKEN_FLOAT_NUM
,QC_TOKEN_FLOAT                     /* SQL-2003-R */
,QC_TOKEN_FLUSH
,QC_TOKEN_FORCE
,QC_TOKEN_FOREIGN                       /* SQL-2003-R */
,QC_TOKEN_FOR                       /* SQL-2003-R */
,QC_TOKEN_FOUND                     /* SQL-2003-R */
,QC_TOKEN_FRAC_SECOND
,QC_TOKEN_FROM
,QC_TOKEN_FULL                          /* SQL-2003-R */
,QC_TOKEN_FULLTEXT
,QC_TOKEN_FUNCTION                  /* SQL-2003-R */
,QC_TOKEN_GE
,QC_TOKEN_GEOMETRYCOLLECTION
,QC_TOKEN_GEOMETRY
,QC_TOKEN_GET_FORMAT                    /* MYSQL-FUNC */
,QC_TOKEN_GLOBAL                    /* SQL-2003-R */
,QC_TOKEN_GRANT                         /* SQL-2003-R */
,QC_TOKEN_GRANTS
,QC_TOKEN_GROUP                     /* SQL-2003-R */
,QC_TOKEN_GROUP_CONCAT
,QC_TOKEN_GT                        /* OPERATOR */
,QC_TOKEN_HANDLER
,QC_TOKEN_HASH
,QC_TOKEN_HAVING                        /* SQL-2003-R */
,QC_TOKEN_HELP
,QC_TOKEN_HEX_NUM
,QC_TOKEN_HIGH_PRIORITY
,QC_TOKEN_HOST
,QC_TOKEN_HOSTS
,QC_TOKEN_HOUR_MICROSECOND
,QC_TOKEN_HOUR_MINUTE
,QC_TOKEN_HOUR_SECOND
,QC_TOKEN_HOUR                      /* SQL-2003-R */
,QC_TOKEN_IDENT
,QC_TOKEN_IDENTIFIED
,QC_TOKEN_IDENT_QUOTED
,QC_TOKEN_IF
,QC_TOKEN_IGNORE
,QC_TOKEN_IGNORE_SERVER_IDS
,QC_TOKEN_IMPORT
,QC_TOKEN_INDEXES
,QC_TOKEN_INDEX
,QC_TOKEN_INFILE
,QC_TOKEN_INITIAL_SIZE
,QC_TOKEN_INNER                     /* SQL-2003-R */
,QC_TOKEN_INOUT                     /* SQL-2003-R */
,QC_TOKEN_INSENSITIVE               /* SQL-2003-R */
,QC_TOKEN_INSERT                        /* SQL-2003-R */
,QC_TOKEN_INSERT_METHOD
,QC_TOKEN_INSTALL
,QC_TOKEN_INTERVAL                  /* SQL-2003-R */
,QC_TOKEN_INTO                          /* SQL-2003-R */
,QC_TOKEN_INT                       /* SQL-2003-R */
,QC_TOKEN_INVOKER
,QC_TOKEN_IN                        /* SQL-2003-R */
,QC_TOKEN_IO
,QC_TOKEN_IPC
,QC_TOKEN_IS                            /* SQL-2003-R */
,QC_TOKEN_ISOLATION                     /* SQL-2003-R */
,QC_TOKEN_ISSUER
,QC_TOKEN_ITERATE
,QC_TOKEN_JOIN                      /* SQL-2003-R */
,QC_TOKEN_KEYS
,QC_TOKEN_KEY_BLOCK_SIZE
,QC_TOKEN_KEY                       /* SQL-2003-N */
,QC_TOKEN_KILL
,QC_TOKEN_LANGUAGE                  /* SQL-2003-R */
,QC_TOKEN_LAST                      /* SQL-2003-N */
,QC_TOKEN_LE                            /* OPERATOR */
,QC_TOKEN_LEADING                       /* SQL-2003-R */
,QC_TOKEN_LEAVES
,QC_TOKEN_LEAVE
,QC_TOKEN_LEFT                          /* SQL-2003-R */
,QC_TOKEN_LESS
,QC_TOKEN_LEVEL
,QC_TOKEN_LEX_HOSTNAME
,QC_TOKEN_LIKE                          /* SQL-2003-R */
,QC_TOKEN_LIMIT
,QC_TOKEN_LINEAR
,QC_TOKEN_LINES
,QC_TOKEN_LINESTRING
,QC_TOKEN_LIST
,QC_TOKEN_LOAD
,QC_TOKEN_LOCAL                     /* SQL-2003-R */
,QC_TOKEN_LOCATOR                   /* SQL-2003-N */
,QC_TOKEN_LOCKS
,QC_TOKEN_LOCK
,QC_TOKEN_LOGFILE
,QC_TOKEN_LOGS
,QC_TOKEN_LONGBLOB
,QC_TOKEN_LONGTEXT
,QC_TOKEN_LONG_NUM
,QC_TOKEN_LONG
,QC_TOKEN_LOOP
,QC_TOKEN_LOW_PRIORITY
,QC_TOKEN_LT                            /* OPERATOR */
,QC_TOKEN_MASTER_CONNECT_RETRY
,QC_TOKEN_MASTER_HOST
,QC_TOKEN_MASTER_LOG_FILE
,QC_TOKEN_MASTER_LOG_POS
,QC_TOKEN_MASTER_PASSWORD
,QC_TOKEN_MASTER_PORT
,QC_TOKEN_MASTER_SERVER_ID
,QC_TOKEN_MASTER_SSL_CAPATH
,QC_TOKEN_MASTER_SSL_CA
,QC_TOKEN_MASTER_SSL_CERT
,QC_TOKEN_MASTER_SSL_CIPHER
,QC_TOKEN_MASTER_SSL_KEY
,QC_TOKEN_MASTER_SSL
,QC_TOKEN_MASTER_SSL_VERIFY_SERVER_CERT
,QC_TOKEN_MASTER
,QC_TOKEN_MASTER_USER
,QC_TOKEN_MASTER_HEARTBEAT_PERIOD
,QC_TOKEN_MATCH                         /* SQL-2003-R */
,QC_TOKEN_MAX_CONNECTIONS_PER_HOUR
,QC_TOKEN_MAX_QUERIES_PER_HOUR
,QC_TOKEN_MAX_ROWS
,QC_TOKEN_MAX_SIZE
,QC_TOKEN_MAX                       /* SQL-2003-N */
,QC_TOKEN_MAX_UPDATES_PER_HOUR
,QC_TOKEN_MAX_USER_CONNECTIONS
,QC_TOKEN_MAX_VALUE                 /* SQL-2003-N */
,QC_TOKEN_MEDIUMBLOB
,QC_TOKEN_MEDIUMINT
,QC_TOKEN_MEDIUMTEXT
,QC_TOKEN_MEDIUM
,QC_TOKEN_MEMORY
,QC_TOKEN_MERGE                     /* SQL-2003-R */
,QC_TOKEN_MESSAGE_TEXT              /* SQL-2003-N */
,QC_TOKEN_MICROSECOND               /* MYSQL-FUNC */
,QC_TOKEN_MIGRATE
,QC_TOKEN_MINUTE_MICROSECOND
,QC_TOKEN_MINUTE_SECOND
,QC_TOKEN_MINUTE                    /* SQL-2003-R */
,QC_TOKEN_MIN_ROWS
,QC_TOKEN_MIN                       /* SQL-2003-N */
,QC_TOKEN_MODE
,QC_TOKEN_MODIFIES                  /* SQL-2003-R */
,QC_TOKEN_MODIFY
,QC_TOKEN_MOD                       /* SQL-2003-N */
,QC_TOKEN_MONTH                     /* SQL-2003-R */
,QC_TOKEN_MULTILINESTRING
,QC_TOKEN_MULTIPOINT
,QC_TOKEN_MULTIPOLYGON
,QC_TOKEN_MUTEX
,QC_TOKEN_MYSQL_ERRNO
,QC_TOKEN_NAMES                     /* SQL-2003-N */
,QC_TOKEN_NAME                      /* SQL-2003-N */
,QC_TOKEN_NATIONAL                  /* SQL-2003-R */
,QC_TOKEN_NATURAL                       /* SQL-2003-R */
,QC_TOKEN_NCHAR_STRING
,QC_TOKEN_NCHAR                     /* SQL-2003-R */
,QC_TOKEN_NDBCLUSTER
,QC_TOKEN_NE                            /* OPERATOR */
,QC_TOKEN_NEG
,QC_TOKEN_NEW                       /* SQL-2003-R */
,QC_TOKEN_NEXT                      /* SQL-2003-N */
,QC_TOKEN_NODEGROUP
,QC_TOKEN_NONE                      /* SQL-2003-R */
,QC_TOKEN_NOT2
,QC_TOKEN_NOT                       /* SQL-2003-R */
,QC_TOKEN_NOW
,QC_TOKEN_NO                        /* SQL-2003-R */
,QC_TOKEN_NO_WAIT
,QC_TOKEN_NO_WRITE_TO_BINLOG
,QC_TOKEN_NULL                      /* SQL-2003-R */
,QC_TOKEN_NUM
,QC_TOKEN_NUMERIC                   /* SQL-2003-R */
,QC_TOKEN_NVARCHAR
,QC_TOKEN_OFFSET
,QC_TOKEN_OLD_PASSWORD
,QC_TOKEN_ON                            /* SQL-2003-R */
,QC_TOKEN_ONE_SHOT
,QC_TOKEN_ONE
,QC_TOKEN_OPEN                      /* SQL-2003-R */
,QC_TOKEN_OPTIMIZE
,QC_TOKEN_OPTIONS
,QC_TOKEN_OPTION                        /* SQL-2003-N */
,QC_TOKEN_OPTIONALLY
,QC_TOKEN_OR2
,QC_TOKEN_ORDER                     /* SQL-2003-R */
,QC_TOKEN_OR_OR                     /* OPERATOR */
,QC_TOKEN_OR                        /* SQL-2003-R */
,QC_TOKEN_OUTER
,QC_TOKEN_OUTFILE
,QC_TOKEN_OUT                       /* SQL-2003-R */
,QC_TOKEN_OWNER
,QC_TOKEN_PACK_KEYS
,QC_TOKEN_PAGE
,QC_TOKEN_PARAM_MARKER
,QC_TOKEN_PARSER
,QC_TOKEN_PARTIAL                       /* SQL-2003-N */
,QC_TOKEN_PARTITIONING
,QC_TOKEN_PARTITIONS
,QC_TOKEN_PARTITION                 /* SQL-2003-R */
,QC_TOKEN_PASSWORD
,QC_TOKEN_PHASE
,QC_TOKEN_PLUGINS
,QC_TOKEN_PLUGIN
,QC_TOKEN_POINT
,QC_TOKEN_POLYGON
,QC_TOKEN_PORT
,QC_TOKEN_POSITION                  /* SQL-2003-N */
,QC_TOKEN_PRECISION                     /* SQL-2003-R */
,QC_TOKEN_PREPARE                   /* SQL-2003-R */
,QC_TOKEN_PRESERVE
,QC_TOKEN_PREV
,QC_TOKEN_PRIMARY                   /* SQL-2003-R */
,QC_TOKEN_PRIVILEGES                    /* SQL-2003-N */
,QC_TOKEN_PROCEDURE                     /* SQL-2003-R */
,QC_TOKEN_PROCESS
,QC_TOKEN_PROCESSLIST
,QC_TOKEN_PROFILE
,QC_TOKEN_PROFILES
,QC_TOKEN_PURGE
,QC_TOKEN_QUARTER
,QC_TOKEN_QUERY
,QC_TOKEN_QUICK
,QC_TOKEN_RANGE                     /* SQL-2003-R */
,QC_TOKEN_READS                     /* SQL-2003-R */
,QC_TOKEN_READ_ONLY
,QC_TOKEN_READ                      /* SQL-2003-N */
,QC_TOKEN_READ_WRITE
,QC_TOKEN_REAL                          /* SQL-2003-R */
,QC_TOKEN_REBUILD
,QC_TOKEN_RECOVER
,QC_TOKEN_REDOFILE
,QC_TOKEN_REDO_BUFFER_SIZE
,QC_TOKEN_REDUNDANT
,QC_TOKEN_REFERENCES                    /* SQL-2003-R */
,QC_TOKEN_REGEXP
,QC_TOKEN_RELAYLOG
,QC_TOKEN_RELAY_LOG_FILE
,QC_TOKEN_RELAY_LOG_POS
,QC_TOKEN_RELAY_THREAD
,QC_TOKEN_RELEASE                   /* SQL-2003-R */
,QC_TOKEN_RELOAD
,QC_TOKEN_REMOVE
,QC_TOKEN_RENAME
,QC_TOKEN_REORGANIZE
,QC_TOKEN_REPAIR
,QC_TOKEN_REPEATABLE                /* SQL-2003-N */
,QC_TOKEN_REPEAT                    /* MYSQL-FUNC */
,QC_TOKEN_REPLACE                       /* MYSQL-FUNC */
,QC_TOKEN_REPLICATION
,QC_TOKEN_REQUIRE
,QC_TOKEN_RESET
,QC_TOKEN_RESIGNAL                  /* SQL-2003-R */
,QC_TOKEN_RESOURCES
,QC_TOKEN_RESTORE
,QC_TOKEN_RESTRICT
,QC_TOKEN_RESUME
,QC_TOKEN_RETURNS                   /* SQL-2003-R */
,QC_TOKEN_RETURN                    /* SQL-2003-R */
,QC_TOKEN_REVOKE                        /* SQL-2003-R */
,QC_TOKEN_RIGHT                         /* SQL-2003-R */
,QC_TOKEN_ROLLBACK                  /* SQL-2003-R */
,QC_TOKEN_ROLLUP                    /* SQL-2003-R */
,QC_TOKEN_ROUTINE                   /* SQL-2003-N */
,QC_TOKEN_ROWS                      /* SQL-2003-R */
,QC_TOKEN_ROW_FORMAT
,QC_TOKEN_ROW                       /* SQL-2003-R */
,QC_TOKEN_RTREE
,QC_TOKEN_SAVEPOINT                 /* SQL-2003-R */
,QC_TOKEN_SCHEDULE
,QC_TOKEN_SCHEMA_NAME               /* SQL-2003-N */
,QC_TOKEN_SECOND_MICROSECOND
,QC_TOKEN_SECOND                    /* SQL-2003-R */
,QC_TOKEN_SECURITY                  /* SQL-2003-N */
,QC_TOKEN_SELECT                    /* SQL-2003-R */
,QC_TOKEN_SENSITIVE                 /* FUTURE-USE */
,QC_TOKEN_SEPARATOR
,QC_TOKEN_SERIALIZABLE              /* SQL-2003-N */
,QC_TOKEN_SERIAL
,QC_TOKEN_SESSION                   /* SQL-2003-N */
,QC_TOKEN_SERVER
,QC_TOKEN_SERVER_OPTIONS
,QC_TOKEN_SET                           /* SQL-2003-R */
,QC_TOKEN_SET_VAR
,QC_TOKEN_SHARE
,QC_TOKEN_SHIFT_LEFT                    /* OPERATOR */
,QC_TOKEN_SHIFT_RIGHT                   /* OPERATOR */
,QC_TOKEN_SHOW
,QC_TOKEN_SHUTDOWN
,QC_TOKEN_SIGNAL                    /* SQL-2003-R */
,QC_TOKEN_SIGNED
,QC_TOKEN_SIMPLE                    /* SQL-2003-N */
,QC_TOKEN_SLAVE
,QC_TOKEN_SMALLINT                      /* SQL-2003-R */
,QC_TOKEN_SNAPSHOT
,QC_TOKEN_SOCKET
,QC_TOKEN_SONAME
,QC_TOKEN_SOUNDS
,QC_TOKEN_SOURCE
,QC_TOKEN_SPATIAL
,QC_TOKEN_SPECIFIC                  /* SQL-2003-R */
,QC_TOKEN_SQLEXCEPTION              /* SQL-2003-R */
,QC_TOKEN_SQLSTATE                  /* SQL-2003-R */
,QC_TOKEN_SQLWARNING                /* SQL-2003-R */
,QC_TOKEN_SQL_BIG_RESULT
,QC_TOKEN_SQL_BUFFER_RESULT
,QC_TOKEN_SQL_CACHE
,QC_TOKEN_SQL_CALC_FOUND_ROWS
,QC_TOKEN_SQL_NO_CACHE
,QC_TOKEN_SQL_SMALL_RESULT
,QC_TOKEN_SQL                       /* SQL-2003-R */
,QC_TOKEN_SQL_THREAD
,QC_TOKEN_SSL
,QC_TOKEN_STARTING
,QC_TOKEN_STARTS
,QC_TOKEN_START                     /* SQL-2003-R */
,QC_TOKEN_STATUS
,QC_TOKEN_STDDEV_SAMP               /* SQL-2003-N */
,QC_TOKEN_STD
,QC_TOKEN_STOP
,QC_TOKEN_STORAGE
,QC_TOKEN_STRAIGHT_JOIN
,QC_TOKEN_STRING
,QC_TOKEN_SUBCLASS_ORIGIN           /* SQL-2003-N */
,QC_TOKEN_SUBDATE
,QC_TOKEN_SUBJECT
,QC_TOKEN_SUBPARTITIONS
,QC_TOKEN_SUBPARTITION
,QC_TOKEN_SUBSTRING                     /* SQL-2003-N */
,QC_TOKEN_SUM                       /* SQL-2003-N */
,QC_TOKEN_SUPER
,QC_TOKEN_SUSPEND
,QC_TOKEN_SWAPS
,QC_TOKEN_SWITCHES
,QC_TOKEN_SYSDATE
,QC_TOKEN_TABLES
,QC_TOKEN_TABLESPACE
,QC_TOKEN_TABLE_REF_PRIORITY
,QC_TOKEN_TABLE                     /* SQL-2003-R */
,QC_TOKEN_TABLE_CHECKSUM
,QC_TOKEN_TABLE_NAME                /* SQL-2003-N */
,QC_TOKEN_TEMPORARY                     /* SQL-2003-N */
,QC_TOKEN_TEMPTABLE
,QC_TOKEN_TERMINATED
,QC_TOKEN_TEXT_STRING
,QC_TOKEN_TEXT
,QC_TOKEN_THAN
,QC_TOKEN_THEN                      /* SQL-2003-R */
,QC_TOKEN_TIMESTAMP                     /* SQL-2003-R */
,QC_TOKEN_TIMESTAMP_ADD
,QC_TOKEN_TIMESTAMP_DIFF
,QC_TOKEN_TIME                      /* SQL-2003-R */
,QC_TOKEN_TINYBLOB
,QC_TOKEN_TINYINT
,QC_TOKEN_TINYTEXT
,QC_TOKEN_TO                        /* SQL-2003-R */
,QC_TOKEN_TRAILING                      /* SQL-2003-R */
,QC_TOKEN_TRANSACTION
,QC_TOKEN_TRIGGERS
,QC_TOKEN_TRIGGER                   /* SQL-2003-R */
,QC_TOKEN_TRIM                          /* SQL-2003-N */
,QC_TOKEN_TRUE                      /* SQL-2003-R */
,QC_TOKEN_TRUNCATE
,QC_TOKEN_TYPES
,QC_TOKEN_TYPE                      /* SQL-2003-N */
,QC_TOKEN_UDF_RETURNS
,QC_TOKEN_ULONGLONG_NUM
,QC_TOKEN_UNCOMMITTED               /* SQL-2003-N */
,QC_TOKEN_UNDEFINED
,QC_TOKEN_UNDERSCORE_CHARSET
,QC_TOKEN_UNDOFILE
,QC_TOKEN_UNDO_BUFFER_SIZE
,QC_TOKEN_UNDO                      /* FUTURE-USE */
,QC_TOKEN_UNICODE
,QC_TOKEN_UNINSTALL
,QC_TOKEN_UNION                     /* SQL-2003-R */
,QC_TOKEN_UNIQUE
,QC_TOKEN_UNKNOWN                   /* SQL-2003-R */
,QC_TOKEN_UNLOCK
,QC_TOKEN_UNSIGNED
,QC_TOKEN_UNTIL
,QC_TOKEN_UPDATE                    /* SQL-2003-R */
,QC_TOKEN_UPGRADE
,QC_TOKEN_USAGE                         /* SQL-2003-N */
,QC_TOKEN_USER                          /* SQL-2003-R */
,QC_TOKEN_USE_FRM
,QC_TOKEN_USE
,QC_TOKEN_USING                         /* SQL-2003-R */
,QC_TOKEN_UTC_DATE
,QC_TOKEN_UTC_TIMESTAMP
,QC_TOKEN_UTC_TIME
,QC_TOKEN_VALUES                        /* SQL-2003-R */
,QC_TOKEN_VALUE                     /* SQL-2003-R */
,QC_TOKEN_VARBINARY
,QC_TOKEN_VARCHAR                       /* SQL-2003-R */
,QC_TOKEN_VARIABLES
,QC_TOKEN_VARIANCE
,QC_TOKEN_VARYING                       /* SQL-2003-R */
,QC_TOKEN_VAR_SAMP
,QC_TOKEN_VIEW                      /* SQL-2003-N */
,QC_TOKEN_WAIT
,QC_TOKEN_WARNINGS
,QC_TOKEN_WEEK
,QC_TOKEN_WHEN                      /* SQL-2003-R */
,QC_TOKEN_WHERE                         /* SQL-2003-R */
,QC_TOKEN_WHILE
,QC_TOKEN_WITH                          /* SQL-2003-R */
,QC_TOKEN_WITH_CUBE                 /* INTERNAL */
,QC_TOKEN_WITH_ROLLUP               /* INTERNAL */
,QC_TOKEN_WORK                      /* SQL-2003-N */
,QC_TOKEN_WRAPPER
,QC_TOKEN_WRITE                     /* SQL-2003-N */
,QC_TOKEN_X509
,QC_TOKEN_XA
,QC_TOKEN_XML
,QC_TOKEN_XOR
,QC_TOKEN_YEAR_MONTH
,QC_TOKEN_YEAR                      /* SQL-2003-R */
,QC_TOKEN_ZEROFILL
	,QC_TOKEN_CLIENT_FLAG
	,QC_TOKEN_GLOBAL_VAR
	,QC_TOKEN_SESSION_VAR
	,QC_TOKEN_BRACKET_OPEN
	,QC_TOKEN_BRACKET_CLOSE
	,QC_TOKEN_PLUS
	,QC_TOKEN_MINUS
	,QC_TOKEN_STAR
	,QC_TOKEN_COMMA
	,QC_TOKEN_DOT
	,QC_TOKEN_SEMICOLON
	,QC_TOKEN_QUESTIONMARK
	,QC_TOKEN_NO_MORE
};


struct st_qc_token_and_value
{
	enum qc_tokens token;
	zval value;
};

PHP_MYSQLND_QC_API struct st_qc_token_and_value mysqlnd_qc_get_token(const char **p, size_t * query_len, const MYSQLND_CHARSET * cset TSRMLS_DC);
PHP_MYSQLND_QC_API smart_str * mysqlnd_qc_query_tokenize(const char * const query, size_t query_len TSRMLS_DC);
PHP_MYSQLND_QC_API smart_str * mysqlnd_qc_query_strip_comments_and_fix_ws(const char * const query, size_t query_len TSRMLS_DC);

#endif /* MYSQLND_QC_TOKENIZE_H */
