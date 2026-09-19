#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/match.hsl" <<'HSL'
enum Message:
    Quit;
    Code(i64);
    Text(str);

fn show(message: Message):
    match message:
        Message.Quit:
            print(0);
        Message.Code(value):
            print(value);
        Message.Text(text):
            print(text);

fn main():
    show(Message.Code(42));
    show(Message.Text("HSL"));
    show(Message.Quit());
HSL
"$HSL" check "$TMP/match.hsl"
"$HSL" build "$TMP/match.hsl" >/dev/null
"$TMP/match" > "$TMP/out"
printf '%s\n' '42' 'HSL' '0' > "$TMP/expected"
cmp "$TMP/expected" "$TMP/out"
cat > "$TMP/non_exhaustive.hsl" <<'HSL'
enum Color:
    Red;
    Blue;
fn main():
    let color: Color = Color.Red;
    match color:
        Color.Red:
            print(1);
HSL
set +e; "$HSL" check "$TMP/non_exhaustive.hsl" >/dev/null 2>"$TMP/err"; s=$?; set -e
test "$s" -ne 0
grep -Fq 'non-exhaustive match for enum Color' "$TMP/err"
echo 'match tests passed'
