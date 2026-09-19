#!/usr/bin/env bash
set -euo pipefail

HSL=${1:?missing hsl executable}
ROOT=${2:?missing project root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

"$HSL" tokens "$ROOT/examples/hello.hsl" > "$TMP/hello.tokens"
grep -q '^Fn' "$TMP/hello.tokens"
grep -q '^Indent' "$TMP/hello.tokens"
grep -q '^TypeI64' "$TMP/hello.tokens"
grep -q '^Dedent' "$TMP/hello.tokens"
grep -q '^Eof' "$TMP/hello.tokens"

printf 'fn main():
    # comment
    return;
' > "$TMP/comment.hsl"
"$HSL" tokens "$TMP/comment.hsl" > "$TMP/comment.tokens"
if grep -q 'comment' "$TMP/comment.tokens"; then
    echo 'comment text was emitted as a token' >&2
    exit 1
fi

printf 'fn main():
  return;
' > "$TMP/bad-indent.hsl"
if "$HSL" tokens "$TMP/bad-indent.hsl" > "$TMP/bad-indent.out" 2> "$TMP/bad-indent.err"; then
    echo 'invalid indentation was accepted' >&2
    exit 1
fi
grep -q 'multiple of four spaces' "$TMP/bad-indent.err"
if grep -q '^Indent' "$TMP/bad-indent.out"; then
    echo 'invalid indentation created an Indent token' >&2
    exit 1
fi

printf 'fn main():
    if true:
        if false:
            return;
    return;
' > "$TMP/nested.hsl"
"$HSL" tokens "$TMP/nested.hsl" > "$TMP/nested.tokens"
if [ "$(grep -c '^Indent' "$TMP/nested.tokens")" -ne 3 ]; then
    echo 'nested indentation token count is incorrect' >&2
    exit 1
fi
if [ "$(grep -c '^Dedent' "$TMP/nested.tokens")" -ne 3 ]; then
    echo 'nested dedentation token count is incorrect' >&2
    exit 1
fi

printf 'fn main():
	return;
' > "$TMP/bad-tab.hsl"
if "$HSL" tokens "$TMP/bad-tab.hsl" > "$TMP/bad-tab.out" 2> "$TMP/bad-tab.err"; then
    echo 'tab indentation was accepted' >&2
    exit 1
fi
grep -q 'tabs are forbidden' "$TMP/bad-tab.err"

printf 'fn main():\n    let message: str = "Hello, Helios";\n    let escaped: str = "line\\nquote: \\"";\n' > "$TMP/strings.hsl"
"$HSL" tokens "$TMP/strings.hsl" > "$TMP/strings.tokens"
if [ "$(grep -c '^String' "$TMP/strings.tokens")" -ne 2 ]; then
    echo 'valid string token count is incorrect' >&2
    exit 1
fi

printf 'fn main():\n    let message: str = "unterminated\n' > "$TMP/unterminated-string.hsl"
if "$HSL" tokens "$TMP/unterminated-string.hsl" > "$TMP/unterminated-string.out" 2> "$TMP/unterminated-string.err"; then
    echo 'unterminated string was accepted' >&2
    exit 1
fi
grep -q 'unterminated string literal' "$TMP/unterminated-string.err"

printf 'fn main():\n    let message: str = "bad\\q";\n' > "$TMP/bad-escape.hsl"
if "$HSL" tokens "$TMP/bad-escape.hsl" > "$TMP/bad-escape.out" 2> "$TMP/bad-escape.err"; then
    echo 'unknown string escape was accepted' >&2
    exit 1
fi
grep -q 'unknown string escape sequence' "$TMP/bad-escape.err"

printf 'fn main():\n    let a = 42;\n    let b = 1_000_000;\n    let c = 0b1010;\n    let d = 0o755;\n    let e = 0xff;\n    let f = 10.5;\n    let g = 2.0f32;\n    let h = 1.5e10;\n' > "$TMP/numbers.hsl"
"$HSL" tokens "$TMP/numbers.hsl" > "$TMP/numbers.tokens"
if [ "$(grep -c '^Integer' "$TMP/numbers.tokens")" -ne 5 ]; then
    echo 'integer token count is incorrect' >&2
    exit 1
fi
if [ "$(grep -c '^Float' "$TMP/numbers.tokens")" -ne 3 ]; then
    echo 'float token count is incorrect' >&2
    exit 1
fi

for literal in '10_' '1__000' '0x' '0b102' '1e'; do
    printf 'fn main():\n    let value = %s;\n' "$literal" > "$TMP/bad-number.hsl"
    if "$HSL" tokens "$TMP/bad-number.hsl" > "$TMP/bad-number.out" 2> "$TMP/bad-number.err"; then
        echo "invalid numeric literal was accepted: $literal" >&2
        exit 1
    fi
done

printf '%b' 'fn main():\n    value += 1;\n    value -= 1;\n    value *= 2;\n    value /= 2;\n    value %= 2;\n    flags &= mask;\n    flags |= mask;\n    flags ^= mask;\n    value <<= 1;\n    value >>= 1;\n    let logic = a && b || !c;\n    let power = value ** 2;\n    let arrow = left => right;\n    let items = [1, 2];\n    let block = {1};\n    @simd;\n' > "$TMP/operators.hsl"
"$HSL" tokens "$TMP/operators.hsl" > "$TMP/operators.tokens"
for kind in PlusEqual MinusEqual StarEqual SlashEqual PercentEqual AmpEqual PipeEqual CaretEqual ShiftLeftEqual ShiftRightEqual AmpAmp PipePipe Bang Power FatArrow LBracket RBracket LBrace RBrace At; do
    grep -q "^$kind" "$TMP/operators.tokens" || {
        echo "missing operator token: $kind" >&2
        exit 1
    }
done

printf '%b' 'fn main():\n    for value in 0..10:\n        print(value);\n    for value in 0..=10:\n        print(value);\n' > "$TMP/ranges.hsl"
"$HSL" tokens "$TMP/ranges.hsl" > "$TMP/ranges.tokens"
grep -q '^For' "$TMP/ranges.tokens"
grep -q '^In' "$TMP/ranges.tokens"
grep -q '^RangeExclusive' "$TMP/ranges.tokens"
grep -q '^RangeInclusive' "$TMP/ranges.tokens"

printf '%b' 'struct Point:\n    x: i64;\n    y: i64;\n\nfn main():\n    let point: Point = Point(20, 22);\n    print(point.x);\n' > "$TMP/structs.hsl"
"$HSL" tokens "$TMP/structs.hsl" > "$TMP/structs.tokens"
grep -q '^Struct' "$TMP/structs.tokens"
grep -q '^Dot' "$TMP/structs.tokens"

printf '%b' 'import math;\nimport geometry_utils;\n\nfn main():\n    print(42);\n' > "$TMP/imports.hsl"

"$HSL" tokens "$TMP/imports.hsl"     > "$TMP/imports.tokens"

test "$(grep -c '^Import' "$TMP/imports.tokens")" -eq 2

printf '%b' 'fn main():\n    let first: List<i64> = List<i64>();\n    let second: List<i64> = move first;\n' > "$TMP/move.hsl"
"$HSL" tokens "$TMP/move.hsl" > "$TMP/move.tokens"
grep -q 'Move' "$TMP/move.tokens"

echo 'lexer tests passed'
echo 'lexer tests passed'
