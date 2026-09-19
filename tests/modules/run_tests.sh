#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/math.hsl" <<'HSL'
fn add(a: i64, b: i64) -> i64:
    return a + b;
HSL

cat > "$TMP/main.hsl" <<'HSL'
import math;

fn main():
    print(math.add(20, 22));
HSL

"$HSL" check "$TMP/main.hsl"

"$HSL" build "$TMP/main.hsl" \
    > "$TMP/build.out"

test -x "$TMP/main"

"$TMP/main" > "$TMP/main.out"

grep -qx '42' "$TMP/main.out"

test ! -e "$TMP/main.generated.cpp"

cat > "$TMP/geometry.hsl" <<'HSL'
struct Point:
    x: i64;
    y: i64;

fn sum(point: Point) -> i64:
    return point.x + point.y;
HSL

cat > "$TMP/struct_main.hsl" <<'HSL'
import geometry;

fn main():
    print(geometry.sum(geometry.Point(20, 22)));
HSL

"$HSL" check "$TMP/struct_main.hsl"

"$HSL" build "$TMP/struct_main.hsl" \
    > "$TMP/struct_build.out"

test -x "$TMP/struct_main"

"$TMP/struct_main" > "$TMP/struct_main.out"

grep -qx '42' "$TMP/struct_main.out"

test ! -e "$TMP/struct_main.generated.cpp"

cat > "$TMP/missing.hsl" <<'HSL'
import absent_module;

fn main():
    print(42);
HSL

set +e

"$HSL" check "$TMP/missing.hsl" \
    > "$TMP/missing.out" \
    2> "$TMP/missing.err"

missing_status=$?

set -e

test "$missing_status" -eq 1

grep -q \
    'cannot find imported module: absent_module' \
    "$TMP/missing.err"

cat > "$TMP/a.hsl" <<'HSL'
import b;

fn value_a() -> i64:
    return 20;
HSL

cat > "$TMP/b.hsl" <<'HSL'
import a;

fn value_b() -> i64:
    return 22;
HSL

cat > "$TMP/cycle_main.hsl" <<'HSL'
import a;

fn main():
    print(42);
HSL

set +e

"$HSL" check "$TMP/cycle_main.hsl" \
    > "$TMP/cycle.out" \
    2> "$TMP/cycle.err"

cycle_status=$?

set -e

test "$cycle_status" -eq 1

grep -q \
    'import cycle detected:' \
    "$TMP/cycle.err"

cat > "$TMP/duplicate_import.hsl" <<'HSL'
import math;
import math;

fn main():
    print(math.add(20, 22));
HSL

"$HSL" check "$TMP/duplicate_import.hsl"

"$HSL" build "$TMP/duplicate_import.hsl" \
    > "$TMP/duplicate_import_build.out"

"$TMP/duplicate_import" \
    > "$TMP/duplicate_import.out"

grep -qx '42' "$TMP/duplicate_import.out"

cat > "$TMP/qualified_main.hsl" <<'HSL'
import geometry;

fn describe(point: geometry.Point) -> i64:
    return geometry.sum(point);

fn main():
    let point: geometry.Point = geometry.Point(20, 22);
    print(describe(point));
HSL

"$HSL" check "$TMP/qualified_main.hsl"

"$HSL" build "$TMP/qualified_main.hsl"     > "$TMP/qualified_build.out"

test -x "$TMP/qualified_main"

"$TMP/qualified_main"     > "$TMP/qualified_main.out"

grep -qx '42' "$TMP/qualified_main.out"

test ! -e "$TMP/qualified_main.generated.cpp"

cat > "$TMP/unknown_type_module.hsl" <<'HSL'
import geometry;

fn main():
    let point: missing.Point = geometry.Point(20, 22);
    print(geometry.sum(point));
HSL

set +e

"$HSL" check "$TMP/unknown_type_module.hsl"     > "$TMP/unknown_type_module.out"     2> "$TMP/unknown_type_module.err"

unknown_type_status=$?

set -e

test "$unknown_type_status" -eq 1

grep -q     'unknown imported module in type: missing'     "$TMP/unknown_type_module.err"

cat > "$TMP/missing_export.hsl" <<'HSL'
import geometry;

fn main():
    let value: geometry.Missing =
        geometry.Point(20, 22);

    print(geometry.sum(value));
HSL

set +e

"$HSL" check "$TMP/missing_export.hsl"     > "$TMP/missing_export.out"     2> "$TMP/missing_export.err"

missing_export_status=$?

set -e

test "$missing_export_status" -eq 1

grep -q     "module 'geometry' has no type named 'Missing'"     "$TMP/missing_export.err"

cat > "$TMP/field_missing_export.hsl" <<'HSL'
import geometry;

struct Shape:
    origin: geometry.Missing;
    size: i64;

fn main():
    print(42);
HSL

set +e

"$HSL" check "$TMP/field_missing_export.hsl"     > "$TMP/field_missing_export.out"     2> "$TMP/field_missing_export.err"

field_missing_status=$?

set -e

test "$field_missing_status" -eq 1

grep -q     "module 'geometry' has no type named 'Missing'"     "$TMP/field_missing_export.err"

cat > "$TMP/parameter_missing_export.hsl" <<'HSL'
import geometry;

fn inspect(value: geometry.Missing) -> i64:
    return 42;

fn main():
    print(42);
HSL

set +e

"$HSL" check "$TMP/parameter_missing_export.hsl"     > "$TMP/parameter_missing_export.out"     2> "$TMP/parameter_missing_export.err"

parameter_missing_status=$?

set -e

test "$parameter_missing_status" -eq 1

grep -q     "module 'geometry' has no type named 'Missing'"     "$TMP/parameter_missing_export.err"

cat > "$TMP/return_missing_export.hsl" <<'HSL'
import geometry;

fn make_point() -> geometry.Missing:
    return geometry.Point(20, 22);

fn main():
    print(42);
HSL

set +e

"$HSL" check "$TMP/return_missing_export.hsl"     > "$TMP/return_missing_export.out"     2> "$TMP/return_missing_export.err"

return_missing_status=$?

set -e

test "$return_missing_status" -eq 1

grep -q     "module 'geometry' has no type named 'Missing'"     "$TMP/return_missing_export.err"

cat > "$TMP/array_geometry.hsl" <<'HSL'
struct Point:
    x: i64;
    y: i64;

fn sum(point: Point) -> i64:
    return point.x + point.y;
HSL

cat > "$TMP/imported_struct_arrays.hsl" <<'HSL'
import array_geometry;

fn main():
    let points: [array_geometry.Point; 2] = [
        array_geometry.Point(10, 20),
        array_geometry.Point(30, 40)
    ];

    print(
        array_geometry.sum(points[0]) +
        array_geometry.sum(points[1])
    );
HSL

"$HSL" check "$TMP/imported_struct_arrays.hsl"
"$HSL" build "$TMP/imported_struct_arrays.hsl" > "$TMP/imported_struct_arrays_build.out"
test -x "$TMP/imported_struct_arrays"
"$TMP/imported_struct_arrays" > "$TMP/imported_struct_arrays.out"
grep -qx '100' "$TMP/imported_struct_arrays.out"
test ! -e "$TMP/imported_struct_arrays.generated.cpp"

cat > "$TMP/imported_missing_struct_array.hsl" <<'HSL'
import array_geometry;

fn main():
    let points: [array_geometry.Missing; 1] = [
        array_geometry.Point(10, 20)
    ];

    print(42);
HSL

set +e
"$HSL" check "$TMP/imported_missing_struct_array.hsl" \
    > "$TMP/imported_missing_struct_array.out" \
    2> "$TMP/imported_missing_struct_array.err"
missing_array_type_status=$?
set -e

test "$missing_array_type_status" -eq 1
grep -Fq \
    "module 'array_geometry' has no type named 'Missing'" \
    "$TMP/imported_missing_struct_array.err"

cat > "$TMP/slice_geometry.hsl" <<'HSL'
struct Point:
    x: i64;
    y: i64;

fn sum(points: [Point]) -> i64:
    var total: i64 = 0;

    for point in points:
        total += point.x + point.y;

    return total;
HSL

cat > "$TMP/imported_struct_slices.hsl" <<'HSL'
import slice_geometry;

fn main():
    let points: [slice_geometry.Point; 3] = [
        slice_geometry.Point(1, 2),
        slice_geometry.Point(10, 20),
        slice_geometry.Point(30, 40)
    ];

    print(slice_geometry.sum(points[1..3]));
HSL

"$HSL" check "$TMP/imported_struct_slices.hsl"

"$HSL" build "$TMP/imported_struct_slices.hsl" \
    > "$TMP/imported_struct_slices_build.out"

test -x "$TMP/imported_struct_slices"

"$TMP/imported_struct_slices" \
    > "$TMP/imported_struct_slices.out"

grep -qx '100' "$TMP/imported_struct_slices.out"

test ! -e "$TMP/imported_struct_slices.generated.cpp"

cat > "$TMP/imported_missing_struct_slice.hsl" <<'HSL'
import slice_geometry;

fn inspect(values: [slice_geometry.Missing]):
    print(42);

fn main():
    let points: [slice_geometry.Point; 1] = [
        slice_geometry.Point(10, 20)
    ];

    inspect(points[..]);
HSL

set +e

"$HSL" check "$TMP/imported_missing_struct_slice.hsl" \
    > "$TMP/imported_missing_struct_slice.out" \
    2> "$TMP/imported_missing_struct_slice.err"

missing_slice_type_status=$?

set -e

test "$missing_slice_type_status" -eq 1

grep -Fq \
    "module 'slice_geometry' has no type named 'Missing'" \
    "$TMP/imported_missing_struct_slice.err"

cat > "$TMP/reference_geometry.hsl" <<'HSL'
struct Point:
    x: i64;
    y: i64;

fn sum(point: &Point) -> i64:
    return point.x + point.y;
HSL

cat > "$TMP/imported_references.hsl" <<'HSL'
import reference_geometry;

fn main():
    let point: reference_geometry.Point =
        reference_geometry.Point(20, 22);

    let reference: &reference_geometry.Point =
        &point;

    print(reference_geometry.sum(reference));
HSL

"$HSL" check "$TMP/imported_references.hsl"

"$HSL" build "$TMP/imported_references.hsl" \
    > "$TMP/imported_references_build.out"

test -x "$TMP/imported_references"

"$TMP/imported_references" \
    > "$TMP/imported_references.out"

grep -qx '42' "$TMP/imported_references.out"

test ! -e "$TMP/imported_references.generated.cpp"

cat > "$TMP/imported_missing_reference.hsl" <<'HSL'
import reference_geometry;

fn inspect(value: &reference_geometry.Missing):
    print(42);

fn main():
    let point: reference_geometry.Point =
        reference_geometry.Point(20, 22);

    inspect(&point);
HSL

set +e

"$HSL" check "$TMP/imported_missing_reference.hsl" \
    > "$TMP/imported_missing_reference.out" \
    2> "$TMP/imported_missing_reference.err"

missing_reference_status=$?

set -e

test "$missing_reference_status" -eq 1

grep -Fq \
    "module 'reference_geometry' has no type named 'Missing'" \
    "$TMP/imported_missing_reference.err"

echo 'module tests passed'
