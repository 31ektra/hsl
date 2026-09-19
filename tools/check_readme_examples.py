#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

if len(sys.argv) != 2:
    print('usage: check_readme_examples.py <hsl-compiler>', file=sys.stderr)
    raise SystemExit(2)
compiler = Path(sys.argv[1]).resolve()
root = Path(__file__).resolve().parents[1]
examples = sorted((root / 'tests' / 'readme_examples').glob('*.hsl'))
if len(examples) < 8:
    raise SystemExit(f'expected at least 8 README examples, found {len(examples)}')
failures = []
for file in examples:
    result = subprocess.run([str(compiler), 'check', str(file)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        failures.append((file.name, result.stdout.strip()))
if failures:
    for name, output in failures:
        print(f'README EXAMPLE {name} FAILED\n{output}', file=sys.stderr)
    raise SystemExit(1)
print(f'README example validation passed: {len(examples)} examples')
