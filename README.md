# HSL

Current stable version: `9.0.0`.

This document defines the complete HSL 9.0.0 command-line interface, lexical structure, syntax, type rules, expressions, statements, declarations, ownership rules, modules, collections, system interfaces, diagnostics, build process, and executable behaviour.

## Complete keyword reference

Keywords are lowercase, case-sensitive reserved tokens. A keyword cannot be used as the name of a variable, parameter, function, field, type, variant, class, structure, or module.

### `fn`

Declares a function.

```hsl
fn add(left: i64, right: i64) -> i64:
    return left + right;
```

The function name follows `fn`. Parameters are enclosed in parentheses and separated by commas. Each parameter has a name, `:`, and a type. A non-void return type follows `->`. The header ends with `:`. The body is an indented block. Calls evaluate their arguments before control enters the function. Every returned value must match the declared return type. Program execution begins in `main`.

### `import`

Imports a module at file scope.

```hsl
import utilities;
```

The statement ends with `;`. Resolution follows the compiler's module search rules. Missing modules, cyclic imports, and conflicting declarations produce diagnostics. Imports are explicit and deterministic.

### `let`

Creates an immutable binding.

```hsl
let answer: i64 = 42;
```

The declaration contains a name, `:`, a type, `=`, an initialiser, and `;`. The initialiser must be compatible with the declared type. The binding cannot later receive a replacement value. Ownership and movement rules still apply to the value.

### `var`

Creates a mutable binding.

```hsl
var count: i64 = 0;
count = 1;
count += 1;
```

The initialiser must match the declared type. A `var` binding may be changed by assignment or a supported compound assignment. Assignment to a `let` binding is rejected.

### `move`

Transfers ownership of a value.

```hsl
let source: List<i64> = List<i64>();
source.push(42);

let destination: List<i64> = move source;
```

After a successful move, the previous binding cannot be read, borrowed, moved again, or otherwise used unless validly reinitialised. Move checking occurs before code generation. A move does not produce an observable null state.

### `return`

Exits the current function immediately.

```hsl
return value;
```

A value-free return is written as `return;`. A returned expression must match the function's declared return type. `return` exits the function, not merely the nearest block, branch, match arm, or loop.

### `if`

Begins conditional execution.

```hsl
if ready:
    print("ready");
```

The condition must have type `bool`. Integers, strings, references, collections, and other values are not implicitly converted to Boolean values.

### `else`

Introduces the alternative branch of an `if`.

```hsl
if ready:
    print("ready");
else:
    print("not ready");
```

Exactly one branch executes. Each branch is checked independently for types, ownership, movement, returns, and reachability.

### `match`

Selects a branch by matching one value against patterns.

```hsl
match value:
    0 => print("zero");
    _ => print("other");
```

The matched expression is evaluated once. Arms use `=>`. If the match produces a value, arm result types must agree. Exhaustiveness, duplicate patterns, unreachable patterns, and variant payloads are checked where applicable.

### `while`

Repeats a block while a Boolean condition remains true.

```hsl
while index < 10:
    index += 1;
```

The condition is evaluated before each iteration. A false initial condition executes the body zero times. `break` exits the loop. `continue` starts its next iteration.

### `for`

Iterates over a supported range or iterable value.

```hsl
for index in 0..10:
    print(index);
```

A loop binding is created for each iteration. Its type is derived from the iterable element type. Ownership and mutation rules remain active in the body.

### `in`

Separates a `for` binding from its iterable expression.

```hsl
for item in items:
    print(item);
```

In stable HSL syntax, `in` is part of the `for` header. It is not a general membership operator.

### `break`

Exits the nearest enclosing loop.

```hsl
while true:
    break;
```

Execution resumes after that loop. `break` is invalid outside a loop and does not return from a function.

### `continue`

Skips the remainder of the current iteration of the nearest enclosing loop.

```hsl
for value in values:
    if value == 0:
        continue;
    print(value);
```

In a `while` loop, execution returns to condition evaluation. In a `for` loop, iteration advances to the next item. It is invalid outside a loop.

### `true`

The Boolean true literal. Its type is `bool`.

```hsl
let enabled: bool = true;
```

### `false`

The Boolean false literal. Its type is `bool`.

```hsl
let enabled: bool = false;
```

It is distinct from `0`, an empty string, and an empty collection. HSL has no implicit truthiness.

### `struct`

Declares a structure type.

```hsl
struct Point:
    x: i64;
    y: i64;
```

Fields have unique names and declared types. Construction must satisfy the structure's field rules. Field access, assignment, movement, and borrowing are type-checked.

### `enum`

Declares a closed set of variants.

```hsl
enum State:
    Ready;
    Waiting;
```

Variant names must be unique within the enumeration. An enum value has one active variant. Variants may carry data where accepted by the grammar. Matching must cover each required variant unless a supported catch-all arm is present.

### `class`

Declares a class type.

```hsl
class Counter:
    value: i64;
```

Fields and members are checked in class scope. Construction, access, mutation, movement, and ownership follow the class rules enforced by semantic analysis and type checking. `class` remains reserved in every context.

### `i64`

Names the signed 64-bit integer type.

```hsl
let value: i64 = -42;
```

It represents whole numbers in the accepted signed 64-bit range. Arithmetic, comparison, bitwise, shift, and assignment operations require compatible operands.

### `u64`

Names the unsigned 64-bit integer type.

```hsl
let value: u64 = 42;
```

It represents non-negative whole numbers in the accepted unsigned 64-bit range. Negative values are invalid. Cross-type conversion must not be assumed.

### `f32`

Names the 32-bit floating-point type.

```hsl
let value: f32 = 3.5f32;
```

The `f32` suffix identifies a single-precision literal where required. Supported arithmetic and comparisons follow floating-point numeric behaviour.

### `f64`

Names the 64-bit floating-point type.

```hsl
let value: f64 = 3.5;
```

Unsuffixed floating-point literals use the language's default floating-point interpretation. Operations require compatible numeric types.

### `bool`

Names the Boolean type. Its only values are `true` and `false`.

```hsl
let ready: bool = true;
```

Conditions require `bool`. Negation uses `!`, conjunction uses `&&`, and disjunction uses `||`.

### `str`

Names the string type.

```hsl
let message: str = "HSL";
```

String literals use double quotation marks. Supported escapes are interpreted by the lexer. Strings can be assigned, moved, passed, returned, printed, compared, and stored where those operations are accepted. HSL has no null string value.

## Build

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The compiler executable is:

```sh
./build/hsl
```

## Commands

### Check a program

Parse, analyse, and type-check without creating an executable.

```sh
./build/hsl check program.hsl
```

Exit status is `0` for a valid program and `1` for diagnostics.

### Print the AST

```sh
./build/hsl ast program.hsl
```

### Build a native executable

```sh
./build/hsl build program.hsl
```

The executable is written beside the source file using the source filename without `.hsl`. Temporary generated C++ is removed after a successful build and retained when native compilation fails.

## Program structure

A program contains imports, structs, and functions. Execution starts in `main`.

```hsl
fn main():
    print(42);
```

Statements end with semicolons. Blocks begin after `:` and use indentation.

## Comments

Comments are not currently part of the stable language surface.

## Primitive types

```hsl
i64
u64
f32
f64
bool
str
void
```

Examples:

```hsl
let signed: i64 = 42;
let unsigned: u64 = 42;
let single: f32 = 3.5f32;
let double: f64 = 3.5;
let enabled: bool = true;
let text: str = "HSL";
```

HSL has no null value or nullable variable state.

## Variables

### Immutable binding

```hsl
let answer: i64 = 42;
```

### Mutable binding

```hsl
var answer: i64 = 40;
answer = 42;
answer += 1;
```

Supported compound assignments:

```hsl
+=
-=
*=
/=
%=
&=
|=
^=
<<=
>>=
```

## Functions

```hsl
fn add(left: i64, right: i64) -> i64:
    return left + right;
```

A function without an explicit return type returns `void`.

```hsl
fn show(value: i64):
    print(value);
```

Call syntax:

```hsl
let result: i64 = add(20, 22);
```

Reference parameters are supported:

```hsl
fn show(value: &i64):
    print(*value);
```

Functions cannot return references.

## Built-in output

```hsl
print(42);
print("HSL");
```

`print` accepts exactly one non-void value and writes a trailing newline.

## Operators

### Arithmetic

```hsl
+
-
*
/
%
**
```

### Comparison

```hsl
==
!=
<
<=
>
>=
```

### Logical

```hsl
&&
||
!
```

### Bitwise

```hsl
&
|
^
~
<<
>>
```

Parentheses control grouping:

```hsl
let value: i64 = (20 + 1) * 2;
```

## Conditionals

```hsl
if value == 42:
    print(42);
else:
    print(0);
```

Conditions must have type `bool`.

## While loops

```hsl
var value: i64 = 0;

while value < 10:
    value += 1;
```

## Range loops

Exclusive end:

```hsl
for value in 0..10:
    print(value);
```

Inclusive end:

```hsl
for value in 0..=10:
    print(value);
```

Range boundaries must be `i64`. Loop variables are immutable and scoped to the loop.

## Loop control

```hsl
while true:
    break;
```

```hsl
for value in 0..10:
    if value == 5:
        continue;
```

`break` and `continue` are valid only inside loops.

## Structs

```hsl
struct Point:
    x: i64;
    y: i64;
```

Construct values in field declaration order:

```hsl
let point: Point = Point(20, 22);
```

Read fields with member access:

```hsl
print(point.x);
```

Mutate fields through a mutable root:

```hsl
var point: Point = Point(20, 22);
point.x = 40;
point.y += 2;
```

Nested field access is supported:

```hsl
rectangle.origin.x = 10;
```

Duplicate structs and duplicate fields are rejected.

## Fixed-size arrays

Array type syntax:

```hsl
[i64; 4]
[Point; 2]
[[i64; 2]; 3]
```

Array literal syntax:

```hsl
let values: [i64; 4] = [10, 20, 30, 40];
```

Multiline arrays:

```hsl
let values: [i64; 4] = [
    10,
    20,
    30,
    40
];
```

Indexing:

```hsl
print(values[2]);
```

Mutable element assignment:

```hsl
var values: [i64; 4] = [10, 20, 30, 40];
values[2] = 12;
```

Literal out-of-bounds indices are rejected during checking. Native indexing uses checked language rules.

## Array iteration

```hsl
let values: [i64; 4] = [10, 20, 30, 40];
var total: i64 = 0;

for value in values:
    total += value;
```

The iterator type is inferred from the array element type. Iterators are immutable. Scalar, struct, and nested arrays are supported.

## Owned values across functions

Owned lists are passed and returned by value. A named owned value must use `move` when crossing a function boundary; a newly returned temporary can flow directly into another declaration or call.

```hsl
fn identity(values: List<i64>) -> List<i64>:
    return move values;

fn main():
    let first: List<i64> = List<i64>();
    let second: List<i64> = identity(move first);
```

Nested lists are supported with whitespace between adjacent closing generic brackets, for example `List<List<i64> >`.

## Explicit ownership moves

Owned values such as lists transfer ownership only with `move`:

```hsl
let first: List<i64> = List<i64>();
first.push(42);

let second: List<i64> = move first;
print(second[0]);
```

After the transfer, `first` is moved and cannot be read, indexed, borrowed, iterated, called through, or moved again. Primitive values and references are not moved. Move-state analysis is currently conservative across conditional and loop bodies.

## Owned lists

HSL provides monomorphized owned lists with `List<T>`.

```hsl
let values: List<i64> = List<i64>();

values.push(20);
values.push(22);

print(values.length());
print(values.capacity());
print(values[0] + values[1]);
```

Remove and return the final element:

```hsl
let value: i64 = values.pop();
```

Iterate over a list:

```hsl
for value in values:
    print(value);
```

List indexing and empty `pop` operations use controlled runtime checks. Lists own their storage and release it automatically. Lists support primitive, string, and struct elements. `clear()` removes every element. Lists of references and nested lists are currently rejected. Copying, movement, function parameters, return values, and list fields are not yet available.

## Read-only slices

Slice type syntax:

```hsl
[i64]
[Point]
```

Full slice:

```hsl
let view: [i64] = values[..];
```

Open tail:

```hsl
let view: [i64] = values[1..];
```

Open head:

```hsl
let view: [i64] = values[..3];
```

Exclusive bounded slice:

```hsl
let view: [i64] = values[1..3];
```

Inclusive end:

```hsl
let view: [i64] = values[1..=2];
```

Slices can be indexed, iterated, and passed to functions:

```hsl
fn sum(values: [i64]) -> i64:
    var total: i64 = 0;

    for value in values:
        total += value;

    return total;
```

Slices are read-only, non-owning views. Slicing temporary values is rejected.

## Immutable references

Reference type syntax:

```hsl
&i64
&Point
&[i64; 4]
```

Borrow a stable value:

```hsl
let value: i64 = 42;
let reference: &i64 = &value;
```

Dereference explicitly:

```hsl
print(*reference);
```

Struct member access through a reference is transparent:

```hsl
fn sum(point: &Point) -> i64:
    return point.x + point.y;
```

Borrowing is allowed for stable lvalues such as variables, fields, and indexed elements. Borrowing temporaries is rejected.

References are immutable and non-null. Assignment through a reference is rejected.

Current safety restrictions:

- functions cannot return references
- struct fields cannot contain references
- arrays cannot contain references
- slices cannot contain references
- references to references are not supported

## Imports and modules

Import a sibling module by its filename without `.hsl`:

```hsl
import geometry;
```

Given `geometry.hsl`:

```hsl
struct Point:
    x: i64;
    y: i64;

fn sum(point: &Point) -> i64:
    return point.x + point.y;
```

Use qualified declarations:

```hsl
import geometry;

fn main():
    let point: geometry.Point =
        geometry.Point(20, 22);

    print(geometry.sum(&point));
```

Qualified imported types work inside arrays, slices, and references:

```hsl
[geometry.Point; 2]
[geometry.Point]
&geometry.Point
```

Missing modules, unknown qualified functions, and unknown exported types produce diagnostics.

## Multiline expressions

Expressions may continue across indented lines:

```hsl
let total: i64 = point.x +
    point.y;
```

Calls may span lines:

```hsl
let point: Point = Point(
    20,
    22
);
```

Arrays and parenthesized expressions may also span lines.

## Diagnostics

Diagnostics include severity, line, column, and a message:

```text
error 4:5: cannot initialize 'value' of type bool with i64
```

`check`, `ast`, and `build` return a nonzero exit status when validation fails.

## Current limitations

HSL currently does not provide:

- null values
- fully recursive generic collections
- generics
- maps or sets
- enums or tagged unions
- pattern matching
- `Option` or `Result`
- mutable references
- reference returns or reference fields
- owned dynamic strings and string-building APIs
- a stable standard library
- a self-hosted compiler

## Project tests

Run all suites:

```sh
ctest --test-dir build --output-on-failure
```

Run one suite:

```sh
ctest --test-dir build -R '^parser$' --output-on-failure
ctest --test-dir build -R '^semantic$' --output-on-failure
ctest --test-dir build -R '^type_checker$' --output-on-failure
ctest --test-dir build -R '^code_generation$' --output-on-failure
ctest --test-dir build -R '^modules$' --output-on-failure
```

## Source layout

```text
include/hsl/    public compiler headers
src/            compiler implementation
tests/          shell-based regression suites
```

Source filenames use lowercase snake_case.

## Bootstrap direction

The current compiler is written in C++ and emits native C++. The path to self-hosting is:

1. owned dynamic collections
2. generics and move semantics
3. compiler-grade strings and string building
4. enums, tagged unions, `Option`, and `Result`
5. exhaustive pattern matching
6. maps and sets
7. filesystem, path, process, and command-line APIs
8. an arena-based AST
9. an HSL lexer and parser
10. a staged self-hosted compiler build

## String operations

Strings provide `length`, `starts_with`, `ends_with`, `contains`, `find`, and checked `slice`. `find` returns the zero-based byte position or `-1` when no match exists. String slicing uses a half-open byte range and terminates with a runtime diagnostic when the range is invalid.

## Self-hosting foundation

`self_hosted/compiler_model.hsl` exercises token records, owned compiler containers, byte-level source inspection, and scanner-oriented functions using only HSL. The native compiler is still the trusted stage-zero compiler until full stage-two and stage-three bootstrap equality is achieved.

## Worked examples

The following examples combine the individual rules into complete fragments. Each example uses explicit types, semicolon-terminated statements, colon-led blocks, and indentation.

### Minimal program

```hsl
fn main():
    print("Hello from HSL");
```

`main` is the entry point. The function has no explicit result type, so it returns `void`. `print` receives one non-void value and appends a newline.

### Immutable and mutable bindings

```hsl
fn main():
    let limit: i64 = 5;
    var current: i64 = 0;

    while current < limit:
        print(current);
        current += 1;
```

`limit` cannot be reassigned because it uses `let`. `current` can be updated because it uses `var`. The loop condition is a `bool` produced by `<`.

### Function parameters and return values

```hsl
fn multiply(left: i64, right: i64) -> i64:
    return left * right;

fn main():
    let result: i64 = multiply(6, 7);
    print(result);
```

The two call arguments are checked against the parameter types. The multiplication result is checked against the declared `i64` return type. The call expression itself therefore has type `i64`.

### Boolean composition

```hsl
fn within(value: i64, minimum: i64, maximum: i64) -> bool:
    return value >= minimum && value <= maximum;

fn main():
    let accepted: bool = within(42, 0, 100);

    if accepted:
        print("accepted");
    else:
        print("rejected");
```

Comparison operators produce `bool`. `&&` combines two Boolean operands. The `if` condition accepts the resulting Boolean value directly.

### Range iteration and loop control

```hsl
fn main():
    for index in 0..10:
        if index == 3:
            continue;

        if index == 8:
            break;

        print(index);
```

The exclusive range `0..10` supplies values from `0` up to, but not including, `10`. `continue` skips the remaining statements in the current iteration. `break` terminates the nearest loop.

### Structure declaration and field access

```hsl
struct Point:
    x: i64;
    y: i64;

fn sum(point: &Point) -> i64:
    return point.x + point.y;

fn main():
    let point: Point = Point(x: 20, y: 22);
    print(sum(&point));
```

`Point` contains two `i64` fields. `sum` receives an immutable reference, accesses the fields transparently, and returns their total. Borrowing with `&point` does not transfer ownership.

### Fixed-size array

```hsl
fn main():
    let values: [i64; 4] = [10, 20, 30, 40];
    print(values[0]);
    print(values[3]);
```

The type records both the element type and length. Every element must be compatible with `i64`. Indexing uses zero-based positions and is protected by the defined runtime checks.

### Array iteration

```hsl
fn main():
    let values: [i64; 4] = [10, 20, 30, 40];

    for value in values:
        print(value);
```

The loop binding receives each element in order. The element type determines the type of `value`.

### Owned list operations

```hsl
fn main():
    let values: List<i64> = List<i64>();

    values.push(20);
    values.push(22);

    print(values.length());
    print(values.capacity());
    print(values[0] + values[1]);

    let final_value: i64 = values.pop();
    print(final_value);
```

The list owns its storage. `push` appends an element, `length` reports the number of elements, `capacity` reports available storage, indexing reads an element, and `pop` removes and returns the final element. Empty `pop` and invalid indexes use controlled runtime checks.

### Read-only slice parameter

```hsl
fn total(values: [i64]) -> i64:
    var result: i64 = 0;

    for value in values:
        result += value;

    return result;

fn main():
    let values: [i64; 4] = [10, 20, 30, 40];
    let view: [i64] = values[1..3];
    print(total(view));
```

The slice `values[1..3]` includes indexes `1` and `2`. A slice is a non-owning, read-only view. The called function can read and iterate over it without taking ownership of the original array.

### Immutable reference

```hsl
fn show(value: &i64):
    print(*value);

fn main():
    let answer: i64 = 42;
    let reference: &i64 = &answer;
    show(reference);
```

`&answer` borrows the value. The reference can be passed to another function. `*value` explicitly dereferences it. References cannot be returned from functions.

### Explicit ownership transfer

```hsl
fn main():
    let first: List<i64> = List<i64>();
    first.push(42);

    let second: List<i64> = move first;
    print(second[0]);
```

Ownership of the list moves from `first` to `second`. Reading, indexing, borrowing, iterating over, or moving `first` again is rejected by move-state analysis. `second` owns the list and remains valid.

### String operations

```hsl
fn main():
    let left: str = "Helios";
    let right: str = "HSL";
    let joined: str = left + right;

    print(joined);
    print(joined.length());
```

String concatenation and string members are available only where documented by the stable surface and accepted by the type checker. Strings are valid values and are never null.

### Importing a module

```hsl
import mathematics;

fn main():
    let result: i64 = mathematics.add(20, 22);
    print(result);
```

Imports are declared at file scope. The module name is resolved through the compiler's module search rules. Imported declarations remain subject to normal visibility, type, ownership, and diagnostic rules.

### Nested control flow

```hsl
fn main():
    var outer: i64 = 0;

    while outer < 3:
        var inner: i64 = 0;

        while inner < 3:
            print(outer * 10 + inner);
            inner += 1;

        outer += 1;
```

Each indented block belongs to the preceding colon-led header. The inner loop has its own mutable binding. A `break` or `continue` would apply to the nearest enclosing loop.

### Grouping and precedence

```hsl
fn main():
    let ungrouped: i64 = 20 + 1 * 2;
    let grouped: i64 = (20 + 1) * 2;

    print(ungrouped);
    print(grouped);
```

Parentheses force the grouped addition to occur before multiplication. Without parentheses, operator precedence determines the evaluation structure.

### Bitwise operations

```hsl
fn main():
    let flags: u64 = 5;
    let mask: u64 = 3;

    print(flags & mask);
    print(flags | mask);
    print(flags ^ mask);
    print(flags << 1);
    print(flags >> 1);
```

Bitwise operands must have compatible integer types. Shift counts and results are checked according to the accepted integer rules.

### Compiler workflow

Check source without producing an executable:

```sh
./build/hsl check program.hsl
```

Inspect the parsed abstract syntax tree:

```sh
./build/hsl ast program.hsl
```

Build a native executable:

```sh
./build/hsl build program.hsl
```

Run the produced executable from the path reported by the compiler or from beside the source file when the default output path is used.

## Final validation

Run `tools/release_gate.sh` for a clean release build, all tests, 2,000 fuzz-smoke inputs, repository auditing, and deterministic generation validation. The current self-hosted source is a compiler foundation, not yet a complete replacement for stage zero; see `docs/completion_status.md`.

## HSL 9.0.0 validation

The full validation command is:

```sh
bash tools/full_release_gate.sh
```

It performs README conformance checks, executable documentation checks, the GNU release gate, 10,000 fuzz inputs by default, a strict warning-clean Clang build, AddressSanitizer tests, UndefinedBehaviourSanitizer tests, deterministic compilation, installation validation, package creation, checksum verification, archive inspection, and repository auditing.

The staged bootstrap compiler and its checksum are documented in `docs/stage_binary.md`.

## HSL 9 operating-system foundation

HSL 9.0.0 adds explicit-width integers (`i8`, `u8`, `i16`, `u16`, `i32`, `u32`, `i128`, `u128`) and pointer-width integers (`isize`, `usize`). These extend the existing `i64` and `u64` types without introducing null variables.

The supported kernel model is hybrid and auditable. HSL implements safe kernel policy, parsers, schedulers, services and userland; a small native layer implements boot entry, privileged instructions, raw pointers, volatile MMIO, interrupt stubs and exact ABI layouts. See `docs/freestanding.md`, `docs/kernel_abi.md` and `runtime/freestanding/hsl_kernel.h`.
