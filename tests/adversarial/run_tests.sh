#!/usr/bin/env bash
set -euo pipefail
HSL=${1:?missing hsl executable}
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
fail(){ set +e; timeout 10 "$HSL" check "$1" >"$1.out" 2>"$1.err"; s=$?; set -e; test "$s" -ne 0; test "$s" -ne 124; }
: > "$TMP/empty.hsl"; fail "$TMP/empty.hsl"; grep -Fq "program must declare a 'main' function" "$TMP/empty.hsl.err"
printf '\xff\xfe\x00fn main():\n' > "$TMP/invalid_utf8.hsl"; fail "$TMP/invalid_utf8.hsl"
printf 'import missing_module;\nfn main():\n    print(42);\n' > "$TMP/missing_module.hsl"; fail "$TMP/missing_module.hsl"
python3 -c 'from pathlib import Path; import sys; n=512; Path(sys.argv[1]).write_text("fn main():\n    print("+"("*n+"42"+")"*n+");\n")' "$TMP/deep.hsl"
set +e; timeout 10 "$HSL" check "$TMP/deep.hsl" >/dev/null 2>&1; s=$?; set -e; test "$s" -ne 124
echo 'adversarial tests passed'
