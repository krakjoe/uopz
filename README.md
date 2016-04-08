UOPZ
====
*User Operations for Zend*

[![Build Status](https://travis-ci.org/krakjoe/uopz.svg?branch=master)](https://travis-ci.org/krakjoe/uopz)

The ```uopz``` extension is focused on providing utilities to aid with unit testing PHP code.

It supports the following activities:

 - Intercepting function execution
 - Intercepting object creation
 - Manipulation of function statics
 - Redefinition of constants
 - Deletion of constants
 - Runtime composition and modification of classes

*Note: All of the above activities are compatible with opcache*

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
* Use mock in place of class for new objects
* @param string class
* @param string mock
**/
void uopz_set_mock(string class, string mock);

/**
* Unset previously set mock
* @param string class
**/
void uopz_unset_mock(string class);

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
