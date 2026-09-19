#!/usr/bin/env python3
import re, subprocess, sys
from pathlib import Path
blocked_names = re.compile(r'(^|/)(\.env($|\.)|secrets?|credentials?)(/|$)|\.(pem|key|p12|pfx|kdbx)$', re.I)
patterns = [
 re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
 re.compile(rb'gh[pousr]_[A-Za-z0-9_]{20,}'),
 re.compile(rb'github_pat_[A-Za-z0-9_]{20,}'),
 re.compile(rb'AKIA[0-9A-Z]{16}'),
]
try:
 files = subprocess.check_output(['git','ls-files','-z'], stderr=subprocess.DEVNULL).split(b'\0')
except subprocess.CalledProcessError:
 files = [str(p).encode() for p in Path('.').rglob('*') if p.is_file() and '.git' not in p.parts and 'build' not in p.parts]
issues=[]
for raw in files:
 if not raw: continue
 name=raw.decode('utf-8','surrogateescape')
 if blocked_names.search(name): issues.append(f'blocked filename: {name}'); continue
 path=Path(name)
 try: data=path.read_bytes()
 except OSError: continue
 for pattern in patterns:
  if pattern.search(data): issues.append(f'possible secret in: {name}'); break
if issues:
 print('\n'.join(issues),file=sys.stderr); sys.exit(1)
print(f'repository audit passed: {len(files)} tracked paths')
