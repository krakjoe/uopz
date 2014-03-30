Posing with uopz
================
*What is posing ...*

**Posing, in PHP, is the ability to replace the implementation of a class, function, or method with a modified implementation at runtime.**

Posing is useful during unit testing to ease testing tightly coupled code, and write more thorough tests.

The following example code is difficult to unit test:

```php
<?php
class Test {
	public function action() {
		$this->dependancy = new Dependancy();
	}
}
?>
```

The ```Test::action``` method is coupled to ```Dependancy``` in such a way that ```Dependancy``` cannot be mocked in the ordinary way.

Declare all the things
======================
*The first solution ...*

```php
<?php
uopz_compose('Something', [], [
	"execute" => function() {
		return true;
	}
]);

class Test {
	public function doSomething() {
		$this->dependancy = new Something();
	}
}
?>
```

The code above allows the programmer to pose as ```Something```, however, the unit testing process will become polluted with code that will never execute if we go at it with a hammer like that.

A more elegant solution would be to declare those classes which are required at test time in a more intelligent way.

Declare what you need
=====================
*A more elegant solution ...*

```uopz``` allows the programmer to overload VM opcodes with a user function; this provides many opportunities to compose classes, interfaces and traits on the fly.

Following is a simple example of doing this:

```php
<?php
uopz_overload(ZEND_FETCH_CLASS, function($class){
	if (!class_exists($class, false)) {
		switch ($class) {
			case "Something": uopz_compose($class, [], [
				"execute" => function(){
					return true;
				}
			]); break;
		}
	}
});

class Test {
	public function doSomething() {
		$this->dependancy 
			= new Something();
		var_dump($this->dependancy->execute());
	}
}

$test = new Test();
$test->doSomething();
?>
```

Real world
==========
*Yeah, but ...*

In the real world, we don't use classes in such a simple way; we ```implement``` and ```extend``` all the things and ```use``` as much as possible.

So, in a real codebase something more along the lines the following example is more appropriate:

```php
<?php
uopz_overload(ZEND_FETCH_CLASS, function(&$class){
	if (!class_exists($class, false)) {
		switch ($class) {
			case "Something": uopz_compose($class, [], [
				"execute" => function(){
					return true;
				}
			]); break;
	
			case "Father": uopz_compose($class, [], [
				"something" => function() {}
			]); break;
		}
	}
});

uopz_overload(ZEND_ADD_INTERFACE, function($class, &$interface){
	if (!interface_exists($interface, false)) {
		switch ($interface) {
			case "IFace": uopz_compose($interface, [], [
				"method" => function(){}
			], [], ZEND_ACC_INTERFACE); break;
		}
	}	
});

uopz_overload(ZEND_ADD_TRAIT, function($class, &$trait){
	if (!trait_exists($trait, false)) {
		switch ($trait) {
			case "Useful": uopz_compose($trait, [], [
				"method" => function(){
					return true;
				}
			], [], ZEND_ACC_TRAIT); break;
		}
	}	
});

/* everything that this class depends on can and will be mocked */
class Test extends Father implements IFace {
	use Useful;
	
	public function doSomething() {
		$this->dependancy 
			= new Something();
		var_dump($this->dependancy->execute());
	}
}

$test = new Test();
$test->doSomething();
var_dump(
	class_parents($test),
	class_implements($test),
	class_uses($test));
?>
```

Functions and Methods
=====================
*Posing as functions ...*

Often, a unit test will invoke functions which ordinarily undertake some exhausting task that is not necessary during unit testing. 

You may even want to invoke failure to test the behaviour of the application under error conditions.

```php
<?php
uopz_function('my', function(){
	/* do not do any work */
	return 5;
});

if (($result = my()) >= 5) {
	printf("Result: %d\n", $result);
}
?>
```

The posing function can call the original implementation in the following way:

```php
<?php
$my = uopz_copy('my');

uopz_function('my', function() use($my) {
	/* call the original implementation
		changing the result in some logical way */
	return $my() * 5;
});

if (($result = my()) >= 5) {
	printf("Result: %d\n", $result);
}
?>
```

**Note: functions overwritten by uopz_function will be backed up automatically, graceful code should call uopz_restore before exiting**
