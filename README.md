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
 - Runtime composition and modification of classes

*Note: All of the above activities are compatible with opcache*

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
void uopz_set_return(string class, string function, mixed value [, bool execute = 0]);

/**
* Provide a return value for an existing function
* @param string function
* @param mixed value
* @param bool execute
* If value is a Closure and execute flag is set, the Closure will
* be executed in place of the existing function
**/
void uopz_set_return(string function, mixed value [, bool execute = 0]);

/**
* Unset a previously set return value
* @param string class
* @param string function
**/
void uopz_unset_return(string class, string function);

/**
* Unset a previously set return value
* @param string function
**/
void uopz_unset_return(string function);

/**
* Get a previously set return value
* @param string class
* @param string function
**/
void uopz_get_return(string class, string function);

/**set_
* Get a previously set return value
* @param string function
**/
void uopz_get_return(string function);

/**
* Use mock in place of class for new objects
* @param string class
* @param mixed mock
* Mock can be an object, or the name of a class
**/
void uopz_set_mock(string class, mixed mock);

/**
* Unset previously set mock
* @param string class
**/
void uopz_unset_mock(string class);

/**
* Get previously set mock for class
* @param string class
**/
void uopz_get_mock(string class);

/**
* Set static variables in method scope
* @param string class
* @param string function
* @param array static
**/
void uopz_set_static(string class, string function, array static);

/**
* Set static variables in function scope
* @param string function
* @param array static
**/
void uopz_set_static(string function, array static);

/**
* Get static variables from method scope
* @param string class
* @param string function
**/
void uopz_get_static(string class, string function) : array;

/**
* Get static variables from function scope
* @param string function
**/
void uopz_get_static(string function) : array;

/**
* Execute hook when entering class::function
* @param string class
* @param string function
**/
void uopz_set_hook(string class, string function, Closure hook);

/**
* Execute hook when entering function
* @param string function
**/
void uopz_set_hook(string function, Closure hook);

/**
* Remove previously set hook on class::function
* @param string class
* @param string function
**/
void uopz_unset_hook(string class, string function);

/**
* Remove previously set hook on function
* @param string function
**/
void uopz_unset_hook(string function);

/**
* Get previously set hook on class::function
* @param string class
* @param string function
**/
void uopz_get_hook(string class, string function);

/**
* Get previously set hook on function
* @param string function
**/
void uopz_get_hook(string function);

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
