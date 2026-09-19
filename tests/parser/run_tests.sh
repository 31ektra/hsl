#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
ROOT=${2:?missing project root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

"$HSL" ast "$ROOT/examples/hello.hsl" > "$TMP/hello.ast"
grep -q '^Program' "$TMP/hello.ast"
grep -q 'Function main' "$TMP/hello.ast"
grep -q 'Let x:i64' "$TMP/hello.ast"
grep -q 'Binary Plus' "$TMP/hello.ast"
grep -q 'Call' "$TMP/hello.ast"

printf '%b' 'fn main():\n    let value = 1 + 2 * 3;\n' > "$TMP/precedence.hsl"
"$HSL" ast "$TMP/precedence.hsl" > "$TMP/precedence.ast"
grep -q 'Binary Plus' "$TMP/precedence.ast"
grep -q 'Binary Star' "$TMP/precedence.ast"

printf '%b' 'fn main():\n    let value = 1\n' > "$TMP/missing_semicolon.hsl"
if "$HSL" ast "$TMP/missing_semicolon.hsl" > "$TMP/error.out" 2> "$TMP/error.err"; then
    echo 'missing semicolon was accepted' >&2
    exit 1
fi
grep -q "expected ';' after variable declaration" "$TMP/error.err"

printf '%b' 'fn add(a: i64, b: i64) -> i64:\n    return a + b;\n\nfn main():\n    var result: i64 = add(20, 22);\n    result += 1;\n    if result == 43:\n        print("correct");\n    else:\n        print("incorrect");\n' > "$TMP/control_flow.hsl"
"$HSL" ast "$TMP/control_flow.hsl" > "$TMP/control_flow.ast"
for expected in 'Function add:i64' 'Parameter a:i64' 'Parameter b:i64' 'Assignment PlusEqual' 'If' 'Else'; do
    grep -q "$expected" "$TMP/control_flow.ast" || {
        echo "missing parser output: $expected" >&2
        exit 1
    }
done

printf '%b' 'fn add(a i64):\n    return;\n' > "$TMP/bad_parameter.hsl"
if "$HSL" ast "$TMP/bad_parameter.hsl" > "$TMP/bad_parameter.out" 2> "$TMP/bad_parameter.err"; then
    echo 'invalid parameter syntax was accepted' >&2
    exit 1
fi
grep -q "expected ':' after parameter name" "$TMP/bad_parameter.err"

printf '%b' 'fn main():\n    while true:\n        break;\n' > "$TMP/loop.hsl"
"$HSL" ast "$TMP/loop.hsl" > "$TMP/loop.ast"
grep -q 'While' "$TMP/loop.ast"
grep -q 'Break' "$TMP/loop.ast"

printf '%b' 'fn main():\n    continue;\n' > "$TMP/bad_continue.hsl"
if "$HSL" ast "$TMP/bad_continue.hsl" > "$TMP/bad_continue.out" 2> "$TMP/bad_continue.err"; then
    echo 'continue outside loop was accepted' >&2
    exit 1
fi
grep -q "can only be used inside a loop" "$TMP/bad_continue.err"

printf '%b' 'fn main():\n    for value in 0..10:\n        print(value);\n' > "$TMP/for_loop.hsl"
"$HSL" ast "$TMP/for_loop.hsl" > "$TMP/for_loop.ast"
grep -q 'For value' "$TMP/for_loop.ast"
grep -q 'Range Exclusive' "$TMP/for_loop.ast"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(20, 22);\n    print(point.x);\n' > "$TMP/structs.hsl"
"$HSL" ast "$TMP/structs.hsl" > "$TMP/structs.ast"
grep -q 'Struct Point' "$TMP/structs.ast"
grep -q 'Field x:i64' "$TMP/structs.ast"
grep -q 'Field y:i64' "$TMP/structs.ast"
grep -q 'Member x' "$TMP/structs.ast"

printf '%b' 'fn main():\n    let first: i64 =\n        42;\n    let second: i64 = 20 +\n        22;\n    print(first + second);\n' > "$TMP/multiline_expression.hsl"

timeout 5 "$HSL" ast "$TMP/multiline_expression.hsl"     > "$TMP/multiline_expression.ast"

grep -q 'Let first:i64' "$TMP/multiline_expression.ast"
grep -q 'Let second:i64' "$TMP/multiline_expression.ast"
grep -q 'Binary Plus' "$TMP/multiline_expression.ast"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(\n        20,\n        22\n    );\n    print(point.x + point.y);\n' > "$TMP/multiline_call.hsl"

timeout 5 "$HSL" ast "$TMP/multiline_call.hsl"     > "$TMP/multiline_call.ast"

grep -q 'Struct Point' "$TMP/multiline_call.ast"
grep -q 'Call' "$TMP/multiline_call.ast"
grep -q 'Member x' "$TMP/multiline_call.ast"

printf '%b' 'fn main():\n    let value: i64 =\n'     > "$TMP/incomplete_initializer.hsl"

set +e
timeout 5 "$HSL" ast "$TMP/incomplete_initializer.hsl"     > "$TMP/incomplete_initializer.out"     2> "$TMP/incomplete_initializer.err"
status=$?
set -e

if [[ $status -eq 0 ]]; then
    echo 'incomplete initializer was accepted' >&2
    exit 1
fi

if [[ $status -eq 124 ]]; then
    echo 'parser hung on incomplete initializer' >&2
    exit 1
fi

grep -q 'expected expression'     "$TMP/incomplete_initializer.err"

printf '%b' 'fn main():\n    @\n'     > "$TMP/unrecoverable_token.hsl"

set +e
timeout 5 "$HSL" ast "$TMP/unrecoverable_token.hsl"     > "$TMP/unrecoverable_token.out"     2> "$TMP/unrecoverable_token.err"
status=$?
set -e

if [[ $status -eq 0 ]]; then
    echo 'invalid token was accepted' >&2
    exit 1
fi

if [[ $status -eq 124 ]]; then
    echo 'parser hung on invalid token' >&2
    exit 1
fi

printf '%b' 'import math;\nimport geometry_utils;\n\nfn main():\n    print(42);\n' > "$TMP/imports.hsl"

"$HSL" ast "$TMP/imports.hsl"     > "$TMP/imports.ast"

grep -q 'Import math' "$TMP/imports.ast"
grep -q 'Import geometry_utils' "$TMP/imports.ast"

printf '%b' 'fn transform(point: geometry.Point) -> geometry.Point:\n    let copy: geometry.Point = point;\n    return copy;\n\nfn main():\n    print(42);\n' > "$TMP/qualified_types.hsl"

"$HSL" ast "$TMP/qualified_types.hsl"     > "$TMP/qualified_types.ast"

grep -q 'Function transform:geometry.Point'     "$TMP/qualified_types.ast"

grep -q 'Parameter point:geometry.Point'     "$TMP/qualified_types.ast"

grep -q 'Let copy:geometry.Point'     "$TMP/qualified_types.ast"

printf '%b' 'fn main():\n    var values: [i64; 4] = [10, 20, 30, 40];\n    values[2] = 12;\n    print(values[0] + values[2]);\n' > "$TMP/arrays.hsl"

"$HSL" ast "$TMP/arrays.hsl" > "$TMP/arrays.ast"

grep -q 'Var values:\[i64; 4\]' "$TMP/arrays.ast"
grep -q 'Array' "$TMP/arrays.ast"
grep -q 'Index' "$TMP/arrays.ast"

printf '%b' 'fn main():\n    let values: [[i64; 2]; 2] = [\n        [10, 20],\n        [30, 40]\n    ];\n    print(values[1][0]);\n' > "$TMP/nested_arrays.hsl"

"$HSL" ast "$TMP/nested_arrays.hsl" > "$TMP/nested_arrays.ast"

grep -q 'Let values:\[\[i64; 2\]; 2\]' "$TMP/nested_arrays.ast"
test "$(grep -c 'Array' "$TMP/nested_arrays.ast")" -ge 3
test "$(grep -c 'Index' "$TMP/nested_arrays.ast")" -ge 2

printf '%b' 'fn main():\n    let values: [i64; 2] = [20, 22];\n    for value in values:\n        print(value);\n' > "$TMP/array_iteration.hsl"

"$HSL" ast "$TMP/array_iteration.hsl" \
    > "$TMP/array_iteration.ast"

grep -q 'For value' "$TMP/array_iteration.ast"
grep -q 'Identifier values' "$TMP/array_iteration.ast"

printf '%b' 'fn inspect(values: [i64]):\n    print(values[0]);\n\nfn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    inspect(values[..]);\n    inspect(values[1..]);\n    inspect(values[..3]);\n    inspect(values[1..3]);\n    inspect(values[1..=2]);\n' > "$TMP/slices.hsl"

"$HSL" ast "$TMP/slices.hsl" > "$TMP/slices.ast"

grep -Fq 'Parameter values:[i64]' "$TMP/slices.ast"
grep -Fq 'Slice Full' "$TMP/slices.ast"
grep -Fq 'Slice ExclusiveStart' "$TMP/slices.ast"
grep -Fq 'Slice ExclusiveEnd' "$TMP/slices.ast"
grep -Fq 'Slice ExclusiveBoth' "$TMP/slices.ast"
grep -Fq 'Slice InclusiveBoth' "$TMP/slices.ast"
test "$(grep -c 'Slice' "$TMP/slices.ast")" -eq 5

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn inspect(value: &i64, point: &Point):\n    print(*value + point.x);\n\nfn main():\n    let value: i64 = 20;\n    let point: Point = Point(22, 0);\n    let value_reference: &i64 = &value;\n    let point_reference: &Point = &point;\n    inspect(value_reference, point_reference);\n' > "$TMP/references.hsl"

"$HSL" ast "$TMP/references.hsl" > "$TMP/references.ast"

grep -Fq 'Parameter value:&i64' "$TMP/references.ast"
grep -Fq 'Parameter point:&Point' "$TMP/references.ast"
grep -Fq 'Let value_reference:&i64' "$TMP/references.ast"
grep -Fq 'Let point_reference:&Point' "$TMP/references.ast"
grep -Fq 'Borrow' "$TMP/references.ast"
grep -Fq 'Dereference' "$TMP/references.ast"

printf '%b' 'fn main():\n    let values: List<i64> = List<i64>();\n    values.push(42);\n    print(values.length());\n' > "$TMP/list_i64.hsl"
"$HSL" ast "$TMP/list_i64.hsl" > "$TMP/list_i64.ast"
grep -Fq 'Let values:List<i64>' "$TMP/list_i64.ast"
grep -Fq 'Member push' "$TMP/list_i64.ast"
grep -Fq 'Member length' "$TMP/list_i64.ast"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let flags: List<bool> = List<bool>();\n    let points: List<Point> = List<Point>();\n    flags.push(true);\n    points.push(Point(20, 22));\n' > "$TMP/generic_lists.hsl"
"$HSL" ast "$TMP/generic_lists.hsl" > "$TMP/generic_lists.ast"
grep -Fq 'Let flags:List<bool>' "$TMP/generic_lists.ast"
grep -Fq 'Let points:List<Point>' "$TMP/generic_lists.ast"

printf '%b' 'fn main():\n    let first: List<i64> = List<i64>();\n    let second: List<i64> = move first;\n    print(second.length());\n' > "$TMP/move.hsl"
"$HSL" ast "$TMP/move.hsl" > "$TMP/move.ast"
grep -q 'Move' "$TMP/move.ast"
grep -Fq 'Let second:List<i64>' "$TMP/move.ast"

printf '%b' 'fn append(\n    values: List<i64>,\n    value: i64\n) -> List<i64>:\n    return move values;\n\nfn main():\n    print(42);\n' > "$TMP/multiline_signature.hsl"
"$HSL" ast "$TMP/multiline_signature.hsl" > "$TMP/multiline_signature.ast"
grep -Fq 'Parameter values:List<i64>' "$TMP/multiline_signature.ast"
grep -Fq 'Parameter value:i64' "$TMP/multiline_signature.ast"

echo 'parser tests passed'
echo 'parser tests passed'
