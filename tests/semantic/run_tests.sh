#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

printf '%b' 'fn add(a: i64, b: i64) -> i64:\n    return a + b;\n\nfn main():\n    var result: i64 = add(20, 22);\n    result += 1;\n    print(result);\n' > "$TMP/valid.hsl"
"$HSL" check "$TMP/valid.hsl"

printf '%b' 'fn main():\n    print(missing);\n' > "$TMP/undefined.hsl"
if "$HSL" check "$TMP/undefined.hsl" 2> "$TMP/undefined.err"; then exit 1; fi
grep -q 'undefined identifier: missing' "$TMP/undefined.err"

printf '%b' 'fn main():\n    let value: i64 = 1;\n    value = 2;\n' > "$TMP/immutable.hsl"
if "$HSL" check "$TMP/immutable.hsl" 2> "$TMP/immutable.err"; then exit 1; fi
grep -q 'cannot assign to immutable variable: value' "$TMP/immutable.err"

printf '%b' 'fn main():\n    unknown();\n' > "$TMP/function.hsl"
if "$HSL" check "$TMP/function.hsl" 2> "$TMP/function.err"; then exit 1; fi
grep -q 'undefined function: unknown' "$TMP/function.err"

printf '%b' 'fn main():\n    for value in 0..10:\n        print(value);\n    print(value);\n' > "$TMP/for_scope.hsl"
if "$HSL" check "$TMP/for_scope.hsl" 2> "$TMP/for_scope.err"; then exit 1; fi
grep -q 'undefined identifier: value' "$TMP/for_scope.err"

printf '%b' 'fn main():\n    for value in 0..10:\n        value = 4;\n' > "$TMP/for_immutable.hsl"
if "$HSL" check "$TMP/for_immutable.hsl" 2> "$TMP/for_immutable.err"; then exit 1; fi
grep -q 'cannot assign to immutable variable: value' "$TMP/for_immutable.err"

printf '%b' 'struct Point:\n    x: i64;\n\nstruct Point:\n    y: i64;\n\nfn main():\n    print(42);\n' > "$TMP/duplicate_struct.hsl"
if "$HSL" check "$TMP/duplicate_struct.hsl" 2> "$TMP/duplicate_struct.err"; then
    exit 1
fi
grep -q 'duplicate struct declaration: Point' "$TMP/duplicate_struct.err"

printf '%b' 'struct Point:\n    x: i64;\n    x: i64;\n\nfn main():\n    print(42);\n' > "$TMP/duplicate_field.hsl"
if "$HSL" check "$TMP/duplicate_field.hsl" 2> "$TMP/duplicate_field.err"; then
    exit 1
fi
grep -q 'duplicate field declaration: x' "$TMP/duplicate_field.err"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(20, 22);\n    point.x = 40;\n' > "$TMP/immutable_field.hsl"
if "$HSL" check "$TMP/immutable_field.hsl" 2> "$TMP/immutable_field.err"; then
    echo 'assignment through immutable struct was accepted' >&2
    exit 1
fi
grep -q 'cannot assign through immutable variable: point' "$TMP/immutable_field.err"

printf '%b' 'fn main():\n    let values: [i64; 2] = [10, 20];\n    values[0] = 42;\n' > "$TMP/immutable_array.hsl"
if "$HSL" check "$TMP/immutable_array.hsl" 2> "$TMP/immutable_array.err"; then
    echo 'assignment through immutable array was accepted' >&2
    exit 1
fi
grep -q 'cannot assign through immutable variable: values' "$TMP/immutable_array.err"

printf '%b' 'fn main():\n    let values: [i64; 2] = [20, 22];\n    for value in values:\n        value = 0;\n' > "$TMP/array_iterator_immutable.hsl"

if "$HSL" check "$TMP/array_iterator_immutable.hsl" \
    2> "$TMP/array_iterator_immutable.err"; then
    echo 'mutable array iterator was accepted' >&2
    exit 1
fi

grep -Fq \
    'cannot assign to immutable variable: value' \
    "$TMP/array_iterator_immutable.err"

printf '%b' 'fn main():\n    let values: [i64; 2] = [20, 22];\n    for value in values:\n        print(value);\n    print(value);\n' > "$TMP/array_iterator_scope.hsl"

if "$HSL" check "$TMP/array_iterator_scope.hsl" \
    2> "$TMP/array_iterator_scope.err"; then
    echo 'array iterator escaped its loop scope' >&2
    exit 1
fi

grep -Fq \
    'undefined identifier: value' \
    "$TMP/array_iterator_scope.err"

printf '%b' 'fn main():\n    var value: i64 = 42;\n    let reference: &i64 = &value;\n    *reference = 10;\n' > "$TMP/reference_assignment.hsl"
if "$HSL" check "$TMP/reference_assignment.hsl" 2> "$TMP/reference_assignment.err"; then
    echo 'assignment through immutable reference was accepted' >&2
    exit 1
fi
grep -Fq 'cannot assign through an immutable reference' "$TMP/reference_assignment.err"

printf '%b' 'fn main():\n    let first: List<i64> = List<i64>();\n    let second: List<i64> = move first;\n    print(first.length());\n' > "$TMP/use_after_move.hsl"
if "$HSL" check "$TMP/use_after_move.hsl" 2> "$TMP/use_after_move.err"; then
    echo 'use after move was accepted' >&2
    exit 1
fi
grep -Fq 'use of moved value: first' "$TMP/use_after_move.err"

printf '%b' 'fn main():\n    let first: List<i64> = List<i64>();\n    let second: List<i64> = move first;\n    let third: List<i64> = move first;\n' > "$TMP/double_move.hsl"
if "$HSL" check "$TMP/double_move.hsl" 2> "$TMP/double_move.err"; then
    echo 'double move was accepted' >&2
    exit 1
fi
grep -Fq 'use of moved value: first' "$TMP/double_move.err"

echo 'semantic tests passed'
