#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

printf '%b' 'fn add(a: i64, b: i64) -> i64:\n    return a + b;\n\nfn main():\n    var result: i64 = add(20, 22);\n    result += 1;\n    if result == 43:\n        print(result);\n' > "$TMP/valid.hsl"
"$HSL" check "$TMP/valid.hsl"

check_error() {
    local name=$1
    local source=$2
    local expected=$3
    printf '%b' "$source" > "$TMP/$name.hsl"
    if "$HSL" check "$TMP/$name.hsl" 2> "$TMP/$name.err"; then
        echo "invalid typed program was accepted: $name" >&2
        exit 1
    fi
    grep -Fq "$expected" "$TMP/$name.err"
}

check_error initializer 'fn main():\n    let value: bool = 42;\n' "cannot initialize 'value' of type bool with i64"
check_error arithmetic 'fn main():\n    let value: i64 = true + 1;\n' 'arithmetic operator requires matching numeric operands'
check_error condition 'fn main():\n    if 42:\n        print(42);\n' 'if condition must have type bool'
check_error argument 'fn add(a: i64) -> i64:\n    return a;\n\nfn main():\n    let value: i64 = add(true);\n' "argument 1 of 'add' expects i64 but received bool"
check_error return_type 'fn answer() -> i64:\n    return false;\n\nfn main():\n    print(answer());\n' "function 'answer' must return i64, not bool"
check_error assignment 'fn main():\n    var value: i64 = 1;\n    value = false;\n' 'cannot assign bool to variable of type i64'

check_error range_boundary 'fn main():\n    for value in false..10:\n        print(value);\n' 'range boundaries must have type i64'

check_error struct_arity 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(20);\n' "struct 'Point' expects 2 field values but received 1"

check_error struct_field_type 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(true, 22);\n' "field 1 of 'Point' expects i64 but received bool"

check_error unknown_field 'struct Point:\n    x: i64;\n\nfn main():\n    let point: Point = Point(20);\n    print(point.z);\n' "struct 'Point' has no field named 'z'"

check_error field_type 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    var point: Point = Point(20, 22);\n    point.x = false;\n' 'cannot assign bool to variable of type i64'

check_error compound_field_type 'struct Label:\n    value: str;\n\nfn main():\n    var label: Label = Label("HSL");\n    label.value += " language";\n' 'compound assignment requires a numeric variable'

check_error array_mixed 'fn main():\n    let values: [i64; 3] = [10, false, 30];\n' 'array elements must have matching types'

check_error array_length 'fn main():\n    let values: [i64; 3] = [10, 20];\n' "cannot initialize 'values' of type [i64; 3] with [i64; 2]"

check_error array_index_type 'fn main():\n    let values: [i64; 2] = [10, 20];\n    print(values[false]);\n' 'array index must have type i64'

check_error array_non_array 'fn main():\n    let value: i64 = 42;\n    print(value[0]);\n' 'indexing requires an array, slice, or list value'

check_error array_bounds 'fn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    print(values[4]);\n' 'array index 4 is out of bounds for length 4'

check_error for_scalar_iterable 'fn main():\n    let value: i64 = 42;\n    for item in value:\n        print(item);\n' 'for loop requires a range or array expression'

check_error for_bool_iterator 'fn main():\n    let values: [bool; 2] = [true, false];\n    for value in values:\n        let number: i64 = value;\n' "cannot initialize 'number' of type i64 with bool"

check_error for_nested_iterator 'fn main():\n    let rows: [[i64; 2]; 1] = [[20, 22]];\n    for row in rows:\n        let value: bool = row[0];\n' "cannot initialize 'value' of type bool with i64"

check_error slice_non_array 'fn main():\n    let value: i64 = 42;\n    print(value[..]);\n' 'slicing requires an array or slice value'

check_error slice_boundary_type 'fn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    let view: [i64] = values[false..3];\n' 'slice boundaries must have type i64'

check_error slice_reverse 'fn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    let view: [i64] = values[3..1];\n' 'slice start must not exceed slice end'

check_error slice_bounds 'fn main():\n    let values: [i64; 4] = [10, 20, 30, 40];\n    let view: [i64] = values[1..5];\n' 'slice end 5 is out of bounds for length 4'

check_error slice_temporary 'fn main():\n    let view: [i64] = [10, 20, 30][1..];\n' 'cannot create a slice from a temporary value'

check_error borrow_temporary 'fn main():\n    let reference: &i64 = &42;\n' 'cannot borrow a temporary value'

check_error dereference_value 'fn main():\n    let value: i64 = 42;\n    print(*value);\n' 'dereference requires a reference value'

check_error nested_reference 'fn main():\n    let value: i64 = 42;\n    let first: &i64 = &value;\n    let second: &i64 = &first;\n' 'references to references are not supported'

check_error reference_return 'fn identity(value: &i64) -> &i64:\n    return value;\n\nfn main():\n    print(42);\n' 'functions cannot return references'

check_error reference_field 'struct Wrapper:\n    value: &i64;\n\nfn main():\n    print(42);\n' 'struct fields cannot have reference types'

check_error reference_array 'fn main():\n    let first: i64 = 20;\n    let second: i64 = 22;\n    let values: [&i64; 2] = [&first, &second];\n' 'arrays cannot contain reference elements'

check_error reference_slice_parameter 'fn inspect(values: [&i64]):\n    print(42);\n\nfn main():\n    print(42);\n' 'slices cannot contain reference elements'

check_error reference_array_parameter 'fn inspect(values: [&i64; 2]):\n    print(42);\n\nfn main():\n    print(42);\n' 'arrays cannot contain reference elements'

check_error list_push_type 'fn main():\n    let values: List<i64> = List<i64>();\n    values.push(false);\n' 'List<i64>.push expects i64'

check_error list_unknown_method 'fn main():\n    let values: List<i64> = List<i64>();\n    values.unknown();\n' "List<i64> has no method named 'unknown'"

check_error list_move 'fn main():\n    let first: List<i64> = List<i64>();\n    let second: List<i64> = first;\n' 'owned values require an explicit move'

check_error list_bool_push_type 'fn main():\n    let values: List<bool> = List<bool>();\n    values.push(42);\n' 'List<bool>.push expects bool'

check_error list_reference_element 'fn main():\n    let value: i64 = 42;\n    let values: List<&i64> = List<&i64>();\n    values.push(&value);\n' 'lists cannot contain reference elements'

check_error move_primitive 'fn main():\n    let first: i64 = 42;\n    let second: i64 = move first;\n' 'move requires an owned value, not i64'

check_error move_temporary 'fn main():\n    let values: List<i64> = move List<i64>();\n' 'move operand must be a variable'

check_error owned_argument_without_move 'fn consume(values: List<i64>):\n    print(values.length());\n\nfn main():\n    let values: List<i64> = List<i64>();\n    consume(values);\n' "owned argument 1 of 'consume' requires an explicit move"

check_error owned_return_without_move 'fn identity(values: List<i64>) -> List<i64>:\n    return values;\n\nfn main():\n    print(42);\n' 'returning an owned value requires an explicit move'

echo 'type checker tests passed'
