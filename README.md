UOPZ
====
*User Operations for Zend*

[![Build Status](https://travis-ci.org/krakjoe/uopz.svg?branch=PHP5)](https://travis-ci.org/krakjoe/uopz)

The ```uopz``` extension exposes Zend engine functionality normally used at compilation and execution time in order to allow modification of the internal structures that represent PHP code.

It supports the following activities:

 - Overloading some Zend opcodes including exit/new and composure opcodes
 - Renaming functions and methods
 - Deletion of functions and methods
 - Copying of functions and methods
 - Redefinition of constants
 - Deletion of constants
 - Runtime composition and modification of classes

*Note: All of the above activities are compatible with opcache, including overloading ZEND_EXIT*

Overloadable Opcodes
====================
*A select few, **useful**, opcodes can be overloaded*

The following opcodes can be overloaded by ```uopz```:

 - ZEND_EXIT
 - ZEND_NEW
 - ZEND_THROW
 - ZEND_FETCH_CLASS
 - ZEND_ADD_TRAIT
 - ZEND_ADD_INTERFACE
 - ZEND_INSTANCEOF

An opcode handler has the following prototype:

```
integer function (&$op1 = null, &$op2 = null)
```

Any modifications to ```op1``` and ```op2``` by the handler will effect Zend's behaviour.

**Note:** You must set `uopz.overloads=1` in your `php.ini` in order to be able to overload opcodes. Failing to do so will result in a `Fatal error: Uncaught exception 'RuntimeException' with message 'overloads are disabled by configuration'`.

Overloading Exit
================
*ZEND_EXIT is a little different ...*

We have to treat ```ZEND_EXIT``` differently, opcache will, by default, optimize away the dead code after an unconditional ```ZEND_EXIT```, during test execution and hackery this is less than optimal. You don't want to disable CFG based optimization in opcache ! 

Returning ```ZEND_USER_OPCODE_RETURN``` from a ```ZEND_EXIT``` overload will result in execution in the outer context continuing, which is all that is required for testing.

*See ```tests/001.phpt``` for detail.*

Returning from Overload
=======================
*Change stuff and return nothing, or ...*

It will usually be the case that an overload does not need to return anything, simply changing the parameters will have the desired effect.

There are howeever, some geeky options, the following values can be returned from overloaded handlers:

 - ZEND_USER_OPCODE_CONTINUE    -> advance 1 opcode and continuue
 - ZEND_USER_OPCODE_ENTER       -> enter into new op_array without recursion
 - ZEND_USER_OPCODE_LEAVE       -> return to calling op_array within the same executor
 - ZEND_USER_OPCODE_DISPATCH    -> call original opcode handler
 - ZEND_USER_OPCODE_DISPATCH_TO -> dispatch to a specific handler (OR'd with ZEND opcode constant)
 - ZEND_USER_OPCODE_RETURN      ->  exit from executor (return from function)

By default, uopz dispatches to the proper handler for the current opcode.

Overload Example
================
*How to use overloading ...*

The following example code shows how to overload ```ZEND_EXIT``` with ```uopz```:

```php
<?php
uopz_overload(ZEND_EXIT, function($status = 0){});

class Test {
	public function method() {
		exit();
	}
}

class Unit {
	public function test() {
		$test = new Test();
		$test->method();
		
		return true;
	} 
}
$unit = new Unit();
var_dump($unit->test());
uopz_overload(ZEND_EXIT, null);
var_dump($unit->test());
echo "failed";
?>
```

Will produce the following:
    
    bool(true)

*Note: Setting an overload to ```null``` effectively removes the overload.*

The next example shows an overload of ```ZEND_ADD_INTERFACE```:

```php
<?php
interface IDefault {
    public function iAmDefault();
}

interface IOther extends IDefault {
    public function iAmOther();
}

uopz_overload(ZEND_ADD_INTERFACE, function($class, &$interface){
    if ($interface == "IDefault") {
        $interface = "IOther";
    }
});

class IClass implements IDefault {

    public function iAmDefault() {}
    public function iAmOther() {}
}

var_dump(
    class_implements("IClass"));
?>
```

Will produce the following:

    array(2) {
        ["IOther"]=>
        string(6) "IOther"
        ["IDefault"]=>
        string(8) "IDefault"
    }

Composition Example
===================
*Compsing clases on the fly ...*

Much is possible, the following example just shows ```uopz_compose``` in action, composing classes from nothing but a list of names:

```php
<?php
class My {
    public function is() {}
}

interface IMy {
	public function is();
}

trait myTrait {
	public function myOtherTrait() {
		
	}
}

uopz_compose("CreatedClass", [
	My::class, 
	IMy::class, 
	myTrait::class, [
		"__construct" => function() {
		
		},
		"hidden" => [
			ZEND_ACC_PRIVATE => function(){}
		],
		"horrible" => [
			ZEND_ACC_PUBLIC | ZEND_ACC_STATIC => function() {
				/* yuk */
			}
		],
		"__callStatic" => [
			ZEND_ACC_PUBLIC | ZEND_ACC_STATIC => function($method, $args) {
				/* magic */
			}
		],
		"is" => function() {
			/* reimplement the interface */
		}
	]
]);

if (class_exists("CreatedClass")) {
	$test = new CreatedClass();

	var_dump(
		class_uses($test),
		class_parents($test),
		class_implements($test));
}
?>
```

Will produce the following:

	array(1) {
	  ["myTrait"]=>
	  string(7) "myTrait"
	}
	array(1) {
	  ["My"]=>
	  string(2) "My"
	}
	array(1) {
	  ["IMy"]=>
	  string(3) "IMy"
	}

API
===
*The PHP API for ```uopz```*

```php
/**
* Overload $opcode with $overload handler
* @param int opcode          a ZEND_* opcode
* @param callable overload   the handler to invoke
**/
void uopz_overload(int opcode, Callable overload);

/**
* Rename $class::$method to $class::$rename
* @param string class
* @param string method
* @param string rename
* Note: if both methods exist, this effectively swaps their names
**/
void uopz_rename(string class, string method, string rename);

/**
* Rename $function to $rename
* @param string function      the function to rename
* @param string rename        the new name for function
* Note: if both functions exist, this effectively swaps their names
**/
void uopz_rename(string function, string rename);

/**
* Makes a backup of $class::$method
* @param string class
* @param string method
* Note: backups are automatically restored on shutdown
**/
void uopz_backup(string class, string method);

/**
* Makes a backup of $function
* @param string function      the function to backup
* Note: backups are automatically restored on shutdown
**/
void uopz_backup(string function);

/**
* Restores previously backed up $class::$method
* @param string class
* @param string method
* @param bool ancestry
**/
void uopz_restore(string class, string method [, bool ancestry = true]);

/**
* Restores previously backed up $function
* @param string function      the function to backup
**/
void uopz_restore(string function);

/**
* Delete $class::$method
* @param string class
* @param string method
**/
void uopz_delete(string class, string method);

/**
* Delete $function
* @param string function
**/
void uopz_delete(string function);

/**
* Declare $class::$method as $handler
* @param string class
* @param string function
* @param Closure handler
* @param int modifiers
* @param bool ancestry
* Note: if the method does not exist it will be created
**/
void uopz_function(string class, string method, Closure handler [, int modifiers = false [, bool ancestry = true]]);

/**
* Declare $function as $handler
* @param string function
* @param Closure handler
* @param int modifiers
* Note: if the function does not exist it will be created
**/
void uopz_function(string function, Closure handler [, int modifiers = 0]);

/**
* Redefine $class::$constant to $value
* @param string class
* @param string constant
* @param mixed  value
* Note: only user constants should be redefined
* Note: if the constant does not exist it will be created
**/
void uopz_redefine(string class, string constant, mixed value);

/**
* Redefine $constant to $value
* @param string constant
* @param mixed  value
* Note: only user constants should be redefined
* Note: if the constant does not exist it will be created
**/
void uopz_redefine(string constant, mixed value);

/**
* Delete $class::$constant
* @param string class
* @param string constant
* Note: only user constants should be undefined
**/
void uopz_undefine(string class, string constant);

/**
* Delete $constant
* @param string constant
* Note: only user constants should be undefined
**/
void uopz_undefine(string constant);

/**
* Makes $class implement $interface
* @param string class
* @param string interface
**/
void uopz_implement(string class, string interface);

/**
* Makes $class extend $parent
* @param string class
* @param string parent
**/
void uopz_extend(string class, string parent);

/**
 * Get or set flags on $class::$method()
 * @param string class
 * @param string method
 * @param int flags
 * Note: use ZEND_ACC_FETCH as flags to get flags
 */
int uopz_flags(string class, string method, int flags);

/**
 * Get or set flags on $method()
 * @param string method
 * @param int flags
 * Note: use ZEND_ACC_FETCH as flags to get flags
 */
int uopz_flags(string function, int flags);

/**
* Composes new class $name using the list of classes that follow, applying all methods provided
* @param string name
* @param array classes
* @param array methods
* @param array properties
* @param int flags
* Note: methods are in the form ```name => function()``` or ```name => [flags => function]```
* Note: properties are in the form ```name => modifiers```, and declared null
**/
void uopz_compose(string name, array classes [, array methods [, array properties [, int flags = ZEND_ACC_CLASS]]]);
```

*Note: For a look at what is possible, see /tests.*

Installation
============

 - ```uopz``` should be loaded *before* opcache.
 
Testing
=======
*Running the test suite*

After ``make`` has executed, run:

	``make test``

You are done reading
====================
That is all !!!
