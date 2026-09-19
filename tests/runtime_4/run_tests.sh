#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/runtime.hsl" <<HSL
fn main():
    let source: str = path_join("$TMP", "source.txt");
    let copy: str = path_join("$TMP", "copy.txt");
    print(fs_write(source, "12345"));
    print(fs_copy(source, copy));
    print(fs_file_size(copy));
    print(i64_to_string(42));
    print(try_parse_i64("123").unwrap());
    print(try_parse_i64("bad").is_none());
    let before: i64 = time_unix_ms();
    sleep_ms(1);
    let after: i64 = time_unix_ms();
    print(after >= before);
    print(process_run_arg("true", "ignored"));
    print(fs_remove_all("$TMP/missing"));
HSL
"$HSL" check "$TMP/runtime.hsl"
"$HSL" build "$TMP/runtime.hsl" >/dev/null
"$TMP/runtime" > "$TMP/out"
printf '%s\n' 1 1 5 42 123 1 1 0 0 > "$TMP/expected"
cmp "$TMP/expected" "$TMP/out"
echo 'HSL 4 runtime tests passed'
