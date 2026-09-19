#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

printf '%b' 'fn add(a: i64, b: i64) -> i64:\n    return a + b;\n\nfn main():\n    let result: i64 = add(20, 22);\n    print(result);\n' > "$TMP/program.hsl"

"$HSL" build "$TMP/program.hsl" > "$TMP/build.out"
test -x "$TMP/program"
"$TMP/program" > "$TMP/program.out"
grep -qx '42' "$TMP/program.out"
test ! -e "$TMP/program.generated.cpp"

printf '%b' 'fn main():\n    var value: i64 = 0;\n    while value < 10:\n        value += 1;\n        if value == 5:\n            continue;\n        if value == 8:\n            break;\n    print(value);\n' > "$TMP/loop.hsl"
"$HSL" build "$TMP/loop.hsl" > "$TMP/loop_build.out"
"$TMP/loop" > "$TMP/loop.out"
grep -qx '8' "$TMP/loop.out"

printf '%b' 'fn main():\n    var total: i64 = 0;\n    for value in 0..10:\n        total += value;\n    print(total);\n' > "$TMP/for_exclusive.hsl"
"$HSL" build "$TMP/for_exclusive.hsl" > "$TMP/for_exclusive_build.out"
"$TMP/for_exclusive" > "$TMP/for_exclusive.out"
grep -qx '45' "$TMP/for_exclusive.out"

printf '%b' 'fn main():\n    var total: i64 = 0;\n    for value in 0..=10:\n        total += value;\n    print(total);\n' > "$TMP/for_inclusive.hsl"
"$HSL" build "$TMP/for_inclusive.hsl" > "$TMP/for_inclusive_build.out"
"$TMP/for_inclusive" > "$TMP/for_inclusive.out"
grep -qx '55' "$TMP/for_inclusive.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn sum(point: Point) -> i64:\n    return point.x + point.y;\n\nfn main():\n    let point: Point = Point(20, 22);\n    print(sum(point));\n' > "$TMP/struct.hsl"

"$HSL" build "$TMP/struct.hsl" > "$TMP/struct_build.out"
"$TMP/struct" > "$TMP/struct.out"
grep -qx '42' "$TMP/struct.out"
test ! -e "$TMP/struct.generated.cpp"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    var point: Point = Point(20, 22);\n    point.x = 40;\n    point.y += 2;\n    print(point.x + point.y);\n' > "$TMP/field_assignment.hsl"

"$HSL" build "$TMP/field_assignment.hsl"     > "$TMP/field_assignment_build.out"

"$TMP/field_assignment" > "$TMP/field_assignment.out"

grep -qx '64' "$TMP/field_assignment.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nstruct Rectangle:\n    origin: Point;\n    width: i64;\n    height: i64;\n\nfn main():\n    var rectangle: Rectangle = Rectangle(Point(0, 0), 20, 22);\n    rectangle.origin.x = 10;\n    print(rectangle.origin.x);\n' > "$TMP/nested_field_assignment.hsl"

"$HSL" build "$TMP/nested_field_assignment.hsl"     > "$TMP/nested_field_assignment_build.out"

"$TMP/nested_field_assignment"     > "$TMP/nested_field_assignment.out"

grep -qx '10' "$TMP/nested_field_assignment.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(\n        20,\n        22\n    );\n    let total: i64 = point.x +\n        point.y;\n    print(total);\n' > "$TMP/multiline.hsl"

"$HSL" build "$TMP/multiline.hsl"     > "$TMP/multiline_build.out"

"$TMP/multiline" > "$TMP/multiline.out"

grep -qx '42' "$TMP/multiline.out"
test ! -e "$TMP/multiline.generated.cpp"

printf '%b' 'fn main():\n    var values: [i64; 4] = [10, 20, 30, 40];\n    values[2] = 12;\n    print(values[0] + values[2]);\n' > "$TMP/arrays.hsl"

"$HSL" build "$TMP/arrays.hsl" > "$TMP/arrays_build.out"
"$TMP/arrays" > "$TMP/arrays.out"
grep -qx '22' "$TMP/arrays.out"
test ! -e "$TMP/arrays.generated.cpp"

printf '%b' 'fn main():\n    let values: [[i64; 2]; 2] = [[10, 20], [30, 40]];\n    print(values[1][0]);\n' > "$TMP/nested_arrays.hsl"

"$HSL" build "$TMP/nested_arrays.hsl" > "$TMP/nested_arrays_build.out"
"$TMP/nested_arrays" > "$TMP/nested_arrays.out"
grep -qx '30' "$TMP/nested_arrays.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    var points: [Point; 2] = [Point(10, 20), Point(30, 40)];\n    points[1].x = 2;\n    print(points[0].x + points[1].x);\n' > "$TMP/struct_arrays.hsl"

"$HSL" build "$TMP/struct_arrays.hsl" > "$TMP/struct_arrays_build.out"
"$TMP/struct_arrays" > "$TMP/struct_arrays.out"
grep -qx '12' "$TMP/struct_arrays.out"
test ! -e "$TMP/struct_arrays.generated.cpp"

printf '%b' 'fn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    var total: i64 = 0;\n    for value in values:\n        total += value;\n    print(total);\n' > "$TMP/array_iteration.hsl"

"$HSL" build "$TMP/array_iteration.hsl" \
    > "$TMP/array_iteration_build.out"
"$TMP/array_iteration" > "$TMP/array_iteration.out"
grep -qx '100' "$TMP/array_iteration.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let points: [Point; 2] = [Point(10, 20), Point(30, 40)];\n    var total: i64 = 0;\n    for point in points:\n        total += point.x + point.y;\n    print(total);\n' > "$TMP/struct_array_iteration.hsl"

"$HSL" build "$TMP/struct_array_iteration.hsl" \
    > "$TMP/struct_array_iteration_build.out"
"$TMP/struct_array_iteration" \
    > "$TMP/struct_array_iteration.out"
grep -qx '100' "$TMP/struct_array_iteration.out"

printf '%b' 'fn main():\n    let rows: [[i64; 2]; 2] = [[10, 20], [30, 40]];\n    var total: i64 = 0;\n    for row in rows:\n        total += row[0] + row[1];\n    print(total);\n' > "$TMP/nested_array_iteration.hsl"

"$HSL" build "$TMP/nested_array_iteration.hsl" \
    > "$TMP/nested_array_iteration_build.out"
"$TMP/nested_array_iteration" \
    > "$TMP/nested_array_iteration.out"
grep -qx '100' "$TMP/nested_array_iteration.out"

printf '%b' 'fn sum(values: [i64]) -> i64:\n    var total: i64 = 0;\n    for value in values:\n        total += value;\n    return total;\n\nfn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    print(sum(values[1..3]));\n' > "$TMP/slices.hsl"

"$HSL" build "$TMP/slices.hsl" > "$TMP/slices_build.out"
"$TMP/slices" > "$TMP/slices.out"
grep -qx '50' "$TMP/slices.out"
test ! -e "$TMP/slices.generated.cpp"

printf '%b' 'fn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    let full: [i64] = values[..];\n    let tail: [i64] = values[2..];\n    let head: [i64] = values[..2];\n    let inclusive: [i64] = values[1..=2];\n    print(full[3] + tail[0] + head[1] + inclusive[0]);\n' > "$TMP/slice_forms.hsl"

"$HSL" build "$TMP/slice_forms.hsl" > "$TMP/slice_forms_build.out"
"$TMP/slice_forms" > "$TMP/slice_forms.out"
grep -qx '110' "$TMP/slice_forms.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn sum(points: [Point]) -> i64:\n    var total: i64 = 0;\n    for point in points:\n        total += point.x + point.y;\n    return total;\n\nfn main():\n    let points: [Point; 3] = [\n        Point(1, 2),\n        Point(10, 20),\n        Point(30, 40)\n    ];\n    print(sum(points[1..3]));\n' > "$TMP/struct_slices.hsl"

"$HSL" build "$TMP/struct_slices.hsl" \
    > "$TMP/struct_slices_build.out"

"$TMP/struct_slices" \
    > "$TMP/struct_slices.out"

grep -qx '100' "$TMP/struct_slices.out"

test ! -e "$TMP/struct_slices.generated.cpp"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn sum(point: &Point) -> i64:\n    return point.x + point.y;\n\nfn main():\n    let point: Point = Point(20, 22);\n    let reference: &Point = &point;\n    print(sum(reference));\n' > "$TMP/references.hsl"
"$HSL" build "$TMP/references.hsl" > "$TMP/references_build.out"
"$TMP/references" > "$TMP/references.out"
grep -qx '42' "$TMP/references.out"
test ! -e "$TMP/references.generated.cpp"

printf '%b' 'fn main():\n    let values: List<i64> = List<i64>();\n    values.push(20);\n    values.push(22);\n    print(values.length());\n    print(values[0] + values[1]);\n' > "$TMP/list_i64.hsl"
"$HSL" build "$TMP/list_i64.hsl" > "$TMP/list_i64_build.out"
"$TMP/list_i64" > "$TMP/list_i64.out"
printf '%s\n' '2' '42' > "$TMP/list_i64.expected"
cmp "$TMP/list_i64.expected" "$TMP/list_i64.out"
test ! -e "$TMP/list_i64.generated.cpp"

printf '%b' 'fn main():\n    let values: List<i64> = List<i64>();\n    values.push(10);\n    values.push(20);\n    values.push(30);\n    values.push(40);\n    var total: i64 = 0;\n    for value in values:\n        total += value;\n    print(total);\n    print(values.pop());\n    print(values.length());\n' > "$TMP/list_iteration.hsl"
"$HSL" build "$TMP/list_iteration.hsl" > "$TMP/list_iteration_build.out"
"$TMP/list_iteration" > "$TMP/list_iteration.out"
printf '%s\n' '100' '40' '3' > "$TMP/list_iteration.expected"
cmp "$TMP/list_iteration.expected" "$TMP/list_iteration.out"

printf '%b' 'fn main():\n    let flags: List<bool> = List<bool>();\n    flags.push(true);\n    flags.push(false);\n    for flag in flags:\n        print(flag);\n    flags.clear();\n    print(flags.length());\n' > "$TMP/generic_bool_list.hsl"
"$HSL" build "$TMP/generic_bool_list.hsl" > "$TMP/generic_bool_list_build.out"
"$TMP/generic_bool_list" > "$TMP/generic_bool_list.out"
printf '%s\n' '1' '0' '0' > "$TMP/generic_bool_list.expected"
cmp "$TMP/generic_bool_list.expected" "$TMP/generic_bool_list.out"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let points: List<Point> = List<Point>();\n    points.push(Point(20, 22));\n    print(points[0].x + points[0].y);\n    let point: Point = points.pop();\n    print(point.x);\n' > "$TMP/generic_struct_list.hsl"
"$HSL" build "$TMP/generic_struct_list.hsl" > "$TMP/generic_struct_list_build.out"
"$TMP/generic_struct_list" > "$TMP/generic_struct_list.out"
printf '%s\n' '42' '20' > "$TMP/generic_struct_list.expected"
cmp "$TMP/generic_struct_list.expected" "$TMP/generic_struct_list.out"

printf '%b' 'fn main():\n    let words: List<str> = List<str>();\n    words.push("Helios");\n    print(words[0]);\n' > "$TMP/generic_string_list.hsl"
"$HSL" build "$TMP/generic_string_list.hsl" > "$TMP/generic_string_list_build.out"
"$TMP/generic_string_list" > "$TMP/generic_string_list.out"
grep -qx 'Helios' "$TMP/generic_string_list.out"

printf '%b' 'fn main():\n    let first: List<i64> = List<i64>();\n    first.push(20);\n    first.push(22);\n    let second: List<i64> = move first;\n    print(second[0] + second[1]);\n' > "$TMP/move_list.hsl"
"$HSL" build "$TMP/move_list.hsl" > "$TMP/move_list_build.out"
"$TMP/move_list" > "$TMP/move_list.out"
grep -qx '42' "$TMP/move_list.out"

printf '%b' 'fn append(values: List<i64>, value: i64) -> List<i64>:\n    values.push(value);\n    return move values;\n\nfn main():\n    let first: List<i64> = List<i64>();\n    first.push(20);\n    let second: List<i64> = append(move first, 22);\n    print(second[0] + second[1]);\n' > "$TMP/owned_functions.hsl"
"$HSL" build "$TMP/owned_functions.hsl" > "$TMP/owned_functions_build.out"
"$TMP/owned_functions" > "$TMP/owned_functions.out"
grep -qx '42' "$TMP/owned_functions.out"

printf '%b' 'fn main():\n    let inner: List<i64> = List<i64>();\n    inner.push(42);\n    let outer: List<List<i64> > = List<List<i64> >();\n    outer.push(move inner);\n    print(outer[0][0]);\n' > "$TMP/nested_lists.hsl"
"$HSL" build "$TMP/nested_lists.hsl" > "$TMP/nested_lists_build.out"
"$TMP/nested_lists" > "$TMP/nested_lists.out"
grep -qx '42' "$TMP/nested_lists.out"

printf '%b' 'fn main():\n    let text: str = "Helios HSL";\n    print(text.length());\n    print(text.starts_with("Helios"));\n    print(text.ends_with("HSL"));\n    print(text.contains("ios"));\n    print(text.find("HSL"));\n    print(text.slice(0, 6));\n' > "$TMP/string_operations.hsl"
"$HSL" build "$TMP/string_operations.hsl" > "$TMP/string_operations_build.out"
"$TMP/string_operations" > "$TMP/string_operations.out"
printf '%s\n' '10' '1' '1' '1' '7' 'Helios' > "$TMP/string_operations.expected"
cmp "$TMP/string_operations.expected" "$TMP/string_operations.out"

printf '%b' 'fn main():\n    let text: str = "HSL";\n    print(text.byte_at(0));\n    print(text.byte_at(1));\n    print(text.byte_at(2));\n' > "$TMP/string_byte_at.hsl"
"$HSL" build "$TMP/string_byte_at.hsl" > "$TMP/string_byte_at_build.out"
"$TMP/string_byte_at" > "$TMP/string_byte_at.out"
printf '%s\n' '72' '83' '76' > "$TMP/string_byte_at.expected"
cmp "$TMP/string_byte_at.expected" "$TMP/string_byte_at.out"

echo 'code generation tests passed'
