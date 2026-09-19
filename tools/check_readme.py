#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
readme = root / 'README.md'
text = readme.read_text(encoding='utf-8')
errors = []

if not text.startswith('# HSL\n'):
    errors.append('README must begin with # HSL')
if 'Current stable version: `9.0.0`.' not in text:
    errors.append('README version is not 9.0.0')
if text.count('```') % 2:
    errors.append('Markdown code fences are unbalanced')

required_headings = [
    'Build', 'Commands', 'Program structure', 'Primitive types', 'Variables',
    'Functions', 'Operators', 'Conditionals', 'While loops', 'Structs',
    'Fixed-size arrays', 'Owned lists', 'Read-only slices',
    'Immutable references', 'Imports and modules', 'Diagnostics',
    'Current limitations', 'Project tests', 'Source layout', 'Final validation',
]
for heading in required_headings:
    if f'## {heading}' not in text:
        errors.append(f'missing heading: {heading}')

keywords = (
    'fn import let var move return if else match while for in break continue '
    'true false struct enum class i64 u64 f32 f64 bool str'
).split()
for keyword in keywords:
    if not re.search(rf'(?m)^### `{re.escape(keyword)}`$', text):
        errors.append(f'missing individual keyword section: {keyword}')

american = {
    'behavior': 'behaviour',
    'analyze': 'analyse',
    'analyzed': 'analysed',
    'initializer': 'initialiser',
    'initialized': 'initialised',
    'initialization': 'initialisation',
}
for american_word, british_word in american.items():
    if re.search(rf'\b{american_word}\b', text, re.IGNORECASE):
        errors.append(f'use British English: {british_word}, not {american_word}')

complete_blocks = []
for match in re.finditer(r'```hsl\n(.*?)\n```', text, re.DOTALL):
    block = match.group(1).strip()
    if re.search(r'(?m)^fn\s+main\s*\(', block):
        complete_blocks.append(block + '\n')

if len(complete_blocks) < 8:
    errors.append(f'expected at least 8 complete fn main examples, found {len(complete_blocks)}')

if errors:
    for error in errors:
        print(f'README ERROR: {error}', file=sys.stderr)
    raise SystemExit(1)
print(f'README validation passed: {len(complete_blocks)} complete examples found')
