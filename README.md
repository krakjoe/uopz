UOPZ
====
*User Operations for Zend*

The ```uopz``` extension exposes Zend engine functionality normally used at compilation and execution time in order to allow modification of the internal structures that represent PHP code.

It supports the following activities:

 - Overloading some Zend opcodes including exit/new and composure opcodes
 - Renaming functions and methods
 - Aliasing functions and methods
 - Deletion of functions and methods
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
 - ZEND_ADD_TRAIT
 - ZEND_ADD_INTERFACE
 - ZEND_INSTANCEOF

An opcode handler has the following prototype:

```
integer function (&$op1 = null, &$op2 = null)
```

Any modifications to ```op1``` and ```op2``` by the handler will effect Zend's behaviour.

Overloading Exit
================
*ZEND_EXIT is a little different ...*

We have to treat ```ZEND_EXIT``` differently, opcache will, by default, optimize away the dead code after an unconditional ```ZEND_EXIT```, during test execution and hackery this is less than optimal. You don't want to disable CFG based optimization in opcache ! 

So ```uopz``` changed ```ZEND_EXIT``` opcodes into opcodes that result in the invokation if the user overload function. These will not be optimized or changes by opcache, and so you can run tests using fully optimized code as you do in production.

Returning ```true``` from a ```ZEND_EXIT``` overload will result in exiting, doing anything else, or nothing, results in continuing with executing normally.

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
uopz_overload(ZEND_EXIT, function(){});

exit();
echo "I will be displayed\n";

uopz_overload(ZEND_EXIT, null);

exit();
echo "I will not be displayed\n";
?>
```

Will produce the following:
    
    I will be displayed

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
	if (strncasecmp($class, "IDefault", strlen("IDefault")) !== false) {
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
	myTrait::class
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
* Alias $class::$method as $class::$alias
* @param string class
* @param string method
* @param string alias
**/
void uopz_alias(string class, string method, string alias);

/**
* Alias $function as $alias
* @param string function
* @param string alias
**/
void uopz_alias(string function, string alias);

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
* Redefine $constant to $value
* @param string constant
* @param mixed  value
* Note: only user constants can be redefined
**/
void uopz_redefine(string constant, mixed value);

/**
* Delete $constant
* @param string constant
* Note: only user constants can be undefined
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
* Composes new class $name using the list of classes that follow
* @param string name
* @param array classes
**/
void uoopz_compose(string name, array classes);
```

*Note: For a look at what is possible, see /tests.*

Installation
============

 - ```uopz``` is a Zend Extension, and so should be loaded with the ```zend_extension``` INI directive. - 
 - ```uopz``` should be loaded *before* opcache.
 
Testing
=======
*Running the test suite*

Running tests with the normal ```make test``` wigs out because this is a ```zend_extension```; use (a sensible variation of) the following command to run tests:

    TEST_PHP_EXECUTABLE=/usr/local/bin/php php run-tests.php

You are done reading
====================
That is all !!!
