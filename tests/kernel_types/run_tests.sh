#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl compiler}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/types.hsl" <<'HSL'
fn i8_id(value: i8) -> i8:
    return value;
fn u8_id(value: u8) -> u8:
    return value;
fn i16_id(value: i16) -> i16:
    return value;
fn u16_id(value: u16) -> u16:
    return value;
fn i32_id(value: i32) -> i32:
    return value;
fn u32_id(value: u32) -> u32:
    return value;
fn i128_id(value: i128) -> i128:
    return value;
fn u128_id(value: u128) -> u128:
    return value;
fn isize_id(value: isize) -> isize:
    return value;
fn usize_id(value: usize) -> usize:
    return value;
fn main():
    print(0);
HSL
"$HSL" check "$tmp/types.hsl"
"$HSL" emit-cpp "$tmp/types.hsl" > "$tmp/types.cpp"
grep -Fq 'std::int8_t' "$tmp/types.cpp"
grep -Fq 'std::uintptr_t' "$tmp/types.cpp"
grep -Fq '__uint128_t' "$tmp/types.cpp"
echo 'HSL 9 kernel-width types passed'
