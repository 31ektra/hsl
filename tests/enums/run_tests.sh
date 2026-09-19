#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/basic.hsl" <<'HSL'
enum Color:
    Red;
    Green;
    Blue;

fn main():
    let color: Color = Color.Green;
    print(color == Color.Green);
HSL
"$HSL" check "$TMP/basic.hsl"
"$HSL" build "$TMP/basic.hsl" >/dev/null
"$TMP/basic" > "$TMP/basic.out"
grep -qx '1' "$TMP/basic.out"
cat > "$TMP/bad_variant.hsl" <<'HSL'
enum Color:
    Red;

fn main():
    let color: Color = Color.Blue;
HSL
set +e
"$HSL" check "$TMP/bad_variant.hsl" >/dev/null 2>"$TMP/bad.err"
status=$?
set -e
test "$status" -ne 0
grep -Fq "enum 'Color' has no variant named 'Blue'" "$TMP/bad.err"
echo 'enum tests passed'
cat > "$TMP/payload.hsl" <<'HSL'
enum Message:
    Quit;
    Code(i64);
    Text(str);

fn main():
    let code: Message = Message.Code(42);
    let text: Message = Message.Text("HSL");
    print(code == Message.Code(42));
    print(text == Message.Text("HSL"));
    print(code == Message.Quit());
HSL
"$HSL" check "$TMP/payload.hsl"
"$HSL" build "$TMP/payload.hsl" >/dev/null
"$TMP/payload" > "$TMP/payload.out"
printf '%s\n' '1' '1' '0' > "$TMP/payload.expected"
cmp "$TMP/payload.expected" "$TMP/payload.out"
