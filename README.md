UOPZ
====
*User Operations for Zend*

[![Build Status](https://travis-ci.org/krakjoe/uopz.svg?branch=master)](https://travis-ci.org/krakjoe/uopz)

The ```uopz``` extension is focused on providing utilities to aid with unit testing PHP code.

It supports the following activities:

 - Intercepting function execution
 - Intercepting object creation
 - Hooking into function execution
 - Manipulation of function statics
 - Manipulation of function flags
 - Redefinition of constants
 - Deletion of constants
 - Runtime creation of functions and methods

*Note: All of the above activities are compatible with opcache*

PHP 7
====

PHP 7 is a different animal to PHP 5, making uopz work for PHP 7 was difficult, and required huge internal changes to uopz.

This has brought about a new API that should allow nearly all the same dark magic as in PHP 5.

Sorry for breaking BC ...

API
===
*The PHP API for ```uopz```*

```php
/**
* Provide a return value for an existing function
* @param string class
* @param string function
* @param mixed value
* @param bool execute
* If value is a Closure and execute flag is set, the Closure will
* be executed in place of the existing function
**/
function uopz_set_return(string class, string function, mixed value [, bool execute = 0]) : bool;

/**
* Provide a return value for an existing function
* @param string function
* @param mixed value
* @param bool execute
* If value is a Closure and execute flag is set, the Closure will
* be executed in place of the existing function
**/
function uopz_set_return(string function, mixed value [, bool execute = 0]) : bool;

/**
* Get a previously set return value
* @param string class
* @param string function
**/
function uopz_get_return(string class, string function) : mixed;

/**
* Get a previously set return value
* @param string function
**/
function uopz_get_return(string function) : mixed;

/**
* Unset a previously set return value
* @param string class
* @param string function
**/
function uopz_unset_return(string class, string function) : bool;

/**
* Unset a previously set return value
* @param string function
**/
function uopz_unset_return(string function) : bool;

/**
* Use mock in place of class for new objects
* @param string class
* @param mixed mock
* Mock can be an object, or the name of a class
**/
function uopz_set_mock(string class, mixed mock);

/**
* Get previously set mock for class
* @param string class
**/
function uopz_get_mock(string class);

/**
* Unset previously set mock
* @param string class
**/
function uopz_unset_mock(string class);

/**
* Get static variables from method scope
* @param string class
* @param string function
**/
function uopz_get_static(string class, string function) : array;

/**
* Get static variables from function scope
* @param string function
**/
function uopz_get_static(string function) : array;

/**
* Set static variables in method scope
* @param string class
* @param string function
* @param array static
**/
function uopz_set_static(string class, string function, array static);

/**
* Set static variables in function scope
* @param string function
* @param array static
**/
function uopz_set_static(string function, array static);

/**
* Execute hook when entering class::function
* @param string class
* @param string function
**/
function uopz_set_hook(string class, string function, Closure hook) : bool;

/**
* Execute hook when entering function
* @param string function
**/
function uopz_set_hook(string function, Closure hook) : bool;

/**
* Get previously set hook on class::function
* @param string class
* @param string function
**/
function uopz_get_hook(string class, string function) : Closure;

/**
* Get previously set hook on function
* @param string function
**/
function uopz_get_hook(string function) : Closure;

/**
* Remove previously set hook on class::function
* @param string class
* @param string function
**/
function uopz_unset_hook(string class, string function) : bool;

/**
* Remove previously set hook on function
* @param string function
**/
function uopz_unset_hook(string function) : bool;

/**
* Add a non-existent method
* @param string class
* @param string function
* @param Closure handler
* @param int flags
**/
function uopz_add_function(string class, string function, Closure handler [, int flags = ZEND_ACC_PUBLIC]) : bool;

/**
* Add a non-existent function
* @param string function
* @param Closure handler
* @param int flags
**/
function uopz_add_function(string function, Closure handler [, int flags = ZEND_ACC_PUBLIC]) : bool;

/**
* Delete a previously added method
* @param string class
* @param string function
**/
function uopz_del_function(string class, string function);

/**
* Delete a previously added function
* @param string function
**/
function uopz_del_function(string function);

/**
* Redefine $class::$constant to $value
* @param string class
* @param string constant
* @param mixed  value
* Note: only user constants should be redefined
* Note: if the constant does not exist it will be created
**/
function uopz_redefine(string class, string constant, mixed value);

/**
* Redefine $constant to $value
* @param string constant
* @param mixed  value
* Note: only user constants should be redefined
* Note: if the constant does not exist it will be created
**/
function uopz_redefine(string constant, mixed value);

/**
* Delete $class::$constant
* @param string class
* @param string constant
* Note: only user constants should be undefined
**/
function uopz_undefine(string class, string constant);

/**
* Delete $constant
* @param string constant
* Note: only user constants should be undefined
**/
function uopz_undefine(string constant);

/**
* Makes $class implement $interface
* @param string class
* @param string interface
**/
function uopz_implement(string class, string interface);

/**
* Makes $class extend $parent
* @param string class
* @param string parent
**/
function uopz_extend(string class, string parent);

/**
 * Get or set flags on $class::$method()
 * @param string class
 * @param string method
 * @param int flags
 */
function uopz_flags(string class, string method [, int flags]) : int;

/**
 * Get or set flags on $method()
 * @param string method
 * @param int flags
 * Note: use ZEND_ACC_FETCH as flags to get flags
 */
function uopz_flags(string function, [, int flags]) : int;

/**
* Set instance property
* @param object instance
* @param string property
* @param mixed value
*/
function uopz_set_property(object instance, string property, mixed value);

/**
* Set static class property
* @param string class
* @param string property
* @param mixed value
*/
function uopz_set_property(string class, string property, mixed value);

/**
* Get instance property
* @param object instance
* @param string property
*/
function uopz_get_property(object instance, string property) : mixed;

/**
* Get static class property
* @param string class
* @param string property
*/
function uopz_get_property(string class, string property) : mixed;
```

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
