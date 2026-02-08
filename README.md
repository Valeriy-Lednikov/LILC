# Simple Interpreted Language

This project is an implementation of a small, simple interpreted programming language.
The language is designed to be minimal, readable, and easy to extend.

It supports variables, constants, procedures, scoped blocks, control flow, and built-in math functions.

---

## Key Features

- Interpreted execution
- Variables and constants
- Expression-based assignments
- Procedures (functions without arguments for now)
- Block-based variable scoping using `{ }` (similar to C)
- Conditional statements
- Loops
- Built-in math functions
- Simple I/O

---

## Language Syntax

### Variables

#### Variable declaration

```
VAR x;
```

Creates a variable `x` with a default value of `0`.

```
VAR x = 5;
```

Creates a variable `x` with an initial value of `5`.

```
VAR x = a + b * 2;
```

Creates a variable and assigns the result of an expression.

---

### Constants

Constants are immutable variables.

```
CONST VAR y = 10;
```

Once declared, the value of a constant cannot be changed.

> Attempting to reassign a constant results in an error.

---

### Assignment

Variables are assigned using the `=` operator.

```
x = 42;
x = x + 1;
```

Assignments support full expressions.

---

### Procedures

Procedures are reusable blocks of code.

```
PROC myFunc {
    PRINTLN "Hello from procedure!";
}
```

Calling a procedure:

```
myFunc;
```

---

### Control Flow

#### IF / ELSE

Supports comparison operators:
`==`, `!=`, `<`, `>`, `<=`, `>=`

```
IF (a + b == 5) {
    PRINTLN "=5";
}
ELSE {
    PRINTLN "Not 5";
}
```

---

#### WHILE loop

```
WHILE (a < 10) {
    a = a + 1;
}
```

The loop executes while the condition evaluates to a non-zero value.

---

### Scope

Blocks defined with `{ }` create a new variable scope.

```
VAR x = 1;

IF (x == 1) {
    VAR x = 10;
    PRINTLN x; // prints 10
}

PRINTLN x; // prints 1
```

---

### Input / Output

#### PRINT

Outputs text or a value without a newline.

```
PRINT "Value: ";
```

#### PRINTLN

Outputs text or a value followed by a newline.

```
PRINTLN x;
```

---

### Program Termination

#### HALT

Immediately stops interpreter execution.

```
HALT;
```

---

## Built-in Math Functions

The language supports a set of built-in mathematical functions:

```
abs, acos, asin, atan, atan2,
ceil, cos, cosh, exp, fac,
floor, ln, log, log10,
ncr, npr, pi, pow,
sin, sinh, sqrt,
tan, tanh
```

Example:

```
VAR angle = 30;
VAR result = sin(angle) * 2;
PRINTLN result;
```

---

## Example Program

```
VAR a;
CONST VAR b = 5;
VAR c = 5 + 5 + 5;

PRINT "Program ";
PRINTLN "Start";

myFunc;

PRINTLN a;
PRINTLN b;
PRINTLN c;

IF (a + b == 5) {
    PRINTLN "=5";
}
ELSE {
    PRINTLN "Not 5";
}

WHILE (a < 10) {
    a = a + 1;
}

PROC myFunc {
    PRINTLN "worked!";
}
```

---

## Notes

- Expressions are evaluated dynamically at runtime
- Non-zero values are treated as `true`
- Zero is treated as `false`
- The language is under active development and syntax may evolve

---

## License

MIT License

Copyright (c) 2026 Valeriy Lednikov
Original project:
https://github.com/Valeriy-Lednikov/LILC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
