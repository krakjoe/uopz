Mocking classes with uopz
=========================
*The problem ...*

Unit testing methods that are tightly coupled is difficult; take for example the following code:

```php
<?php
class Test {
	public function doSomething() {
		$this->dependancy = new Something();
	}
}
?>
```

Running a unit test on ```doSomething``` is difficult if ```Something``` is not declared, moreover ```Something``` cannot be an instance of a mock object.

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

The code above allows the programmer to mock ```Something```, however, the unit testing process will become polluted with code that will never execute if we go at it with a hammer like that.

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
