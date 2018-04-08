--TEST--
mysqlnd_qc_handler_default reflection
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--INI--
apc.enabled=1
apc.enable_cli=1
apc.use_request_time=0
apc.slam_defense=0
--FILE--
<?php
	require_once("reflection_tools.inc");
	$class = new ReflectionClass('mysqlnd_qc_handler_default');
	inspectClass($class);
	print "done!";
?>
--EXPECTF--
Inspecting class 'mysqlnd_qc_handler_default'
isInternal: yes
isUserDefined: no
isInstantiable: yes
isInterface: no
isAbstract: no
isFinal: no
isIteratable: no
Modifiers: '0'
Parent Class: 'Interface [ <internal:mysqlnd_qc> interface mysqlnd_qc_handler ] {

  - Constants [0] {
  }

  - Static properties [0] {
  }

  - Static methods [0] {
  }

  - Properties [0] {
  }

  - Methods [9] {
    Method [ <internal:mysqlnd_qc> abstract public method get_hash_key ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method find_in_cache ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method return_to_cache ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method add_to_cache ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method update_cache_stats ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method get_stats ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method clear_cache ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method init ] {
    }

    Method [ <internal:mysqlnd_qc> abstract public method shutdown ] {
    }
  }
}
'
Extension: 'mysqlnd_qc'

Inspecting method 'add_to_cache'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'clear_cache'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'find_in_cache'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'get_hash_key'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'get_stats'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'init'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'return_to_cache'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'shutdown'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting method 'update_cache_stats'
isFinal: no
isAbstract: no
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isConstructor: no
isDestructor: no
isInternal: yes
isUserDefined: no
returnsReference: no
Modifiers: 264
Number of Parameters: 0
Number of Required Parameters: 0

Inspecting property 'entries'
isPublic: yes
isPrivate: no
isProtected: no
isStatic: no
isDefault: yes
Modifiers: 256
Default property 'entries'
done!