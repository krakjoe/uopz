Disassembler
============
*Tearing apart PHP, and sticking it back together, since 2015*

__These functions are experimental and are still being developed__

There are multiple tools for printing or dumping opcodes, vld is one, phpdbg is another. 

However, none of these tools allow programmatic interaction.

uopz for PHP7 is able to disassemble PHP code into a representation that is understandable for humans, then reassemble it for Zend.

```php
function uopz_disassemble(string class, string method) : array;
function uopz_disassemble(string function) : array;
function uopz_disassemble(Closure closure) : array;
function uopz_assemble(array disassembly) : Closure;
```

Whenever I ask my other half why Lord Voldemort has no nose, she replies "he done something really bad" ... *prepare to loose your nose* ...

The best way to learn about the format of a disassembly is by trying it for yourself, however, the following simple example should give you a rough idea of what to expect:

```php
function add(int $a, int $b) : int {
	return $a + $b;
}
```

The disassembly for the code above is thus:

```
array(12) {
  ["name"]=>
  string(9) "{closure}"
  ["flags"]=>
  array(1) {
    [0]=>
    string(6) "public"
  }
  ["nargs"]=>
  int(2)
  ["rnargs"]=>
  int(2)
  ["arginfo"]=>
  array(3) {
    [-1]=>
    array(1) {
      ["type"]=>
      string(3) "int"
    }
    [0]=>
    array(5) {
      ["name"]=>
      string(1) "a"
      ["type"]=>
      string(3) "int"
      ["reference"]=>
      bool(false)
      ["null"]=>
      bool(false)
      ["variadic"]=>
      bool(false)
    }
    [1]=>
    array(5) {
      ["name"]=>
      string(1) "b"
      ["type"]=>
      string(3) "int"
      ["reference"]=>
      bool(false)
      ["null"]=>
      bool(false)
      ["variadic"]=>
      bool(false)
    }
  }
  ["opcodes"]=>
  array(7) {
    [0]=>
    array(3) {
      ["opcode"]=>
      string(9) "ZEND_RECV"
      ["result"]=>
      array(2) {
        ["type"]=>
        string(2) "cv"
        ["num"]=>
        int(0)
      }
      ["lineno"]=>
      int(2)
    }
    [1]=>
    array(3) {
      ["opcode"]=>
      string(9) "ZEND_RECV"
      ["result"]=>
      array(2) {
        ["type"]=>
        string(2) "cv"
        ["num"]=>
        int(1)
      }
      ["lineno"]=>
      int(2)
    }
    [2]=>
    array(5) {
      ["opcode"]=>
      string(8) "ZEND_ADD"
      ["op1"]=>
      array(2) {
        ["type"]=>
        string(2) "cv"
        ["num"]=>
        int(0)
      }
      ["op2"]=>
      array(2) {
        ["type"]=>
        string(2) "cv"
        ["num"]=>
        int(1)
      }
      ["result"]=>
      array(2) {
        ["type"]=>
        string(3) "tmp"
        ["num"]=>
        int(0)
      }
      ["lineno"]=>
      int(3)
    }
    [3]=>
    array(3) {
      ["opcode"]=>
      string(23) "ZEND_VERIFY_RETURN_TYPE"
      ["op1"]=>
      array(2) {
        ["type"]=>
        string(3) "tmp"
        ["num"]=>
        int(0)
      }
      ["lineno"]=>
      int(3)
    }
    [4]=>
    array(3) {
      ["opcode"]=>
      string(11) "ZEND_RETURN"
      ["op1"]=>
      array(2) {
        ["type"]=>
        string(3) "tmp"
        ["num"]=>
        int(0)
      }
      ["lineno"]=>
      int(3)
    }
    [5]=>
    array(2) {
      ["opcode"]=>
      string(23) "ZEND_VERIFY_RETURN_TYPE"
      ["lineno"]=>
      int(4)
    }
    [6]=>
    array(4) {
      ["opcode"]=>
      string(11) "ZEND_RETURN"
      ["op1"]=>
      array(2) {
        ["type"]=>
        string(8) "constant"
        ["num"]=>
        int(0)
      }
      ["ext"]=>
      int(4294967295)
      ["lineno"]=>
      int(4)
    }
  }
  ["vars"]=>
  array(2) {
    [0]=>
    string(1) "a"
    [1]=>
    string(1) "b"
  }
  ["literals"]=>
  array(1) {
    [0]=>
    NULL
  }
  ["file"]=>
  string(22) "/usr/src/uopz/test.php"
  ["start"]=>
  int(2)
  ["end"]=>
  int(4)
  ["T"]=>
  int(1)
}
```

I'm not going to explain every single element of every single array, it should be self explanatory ...

The disassembly includes the folowing information:

  - scope (a class name, or omitted)
  - name  (function name)
  - this  (index of $this in vars)
  - flags (modifiers and additional info)
  - argument info (num args, required num args, arg info)
  - opcodes
  - literals (constants)
  - vars (compiled, temp etc)
  - static vars (includes use'd vars)
  - break/continue info
  - try/catch info

This is all the information contained in a ```zend_op_array```, Zend's internal representation of PHP code.

```uopz_assemble``` will take an array returned by ```uopz_disassemble``` and return a ```Closure``` ... 

*The author of uopz does not accept responsability for scary things that are written using uopz_disassemble or uopz_assemble ...*
