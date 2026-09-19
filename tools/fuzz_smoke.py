#!/usr/bin/env python3
from pathlib import Path
import os,random,subprocess,sys,tempfile
hsl=Path(sys.argv[1]).resolve(); count=int(sys.argv[2]) if len(sys.argv)>2 else 1000
rng=random.Random(0x48534c); chars=b'abcdefghijklmnopqrstuvwxyz0123456789_()[]{}:;,.+-*/%&|!<>=\n \t"'
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'fuzz.hsl'
 for i in range(count):
  p.write_bytes(os.urandom(rng.randrange(512)) if i%10==0 else bytes(rng.choice(chars) for _ in range(rng.randrange(2048))))
  for cmd in ('tokens','ast','check'):
   r=subprocess.run([hsl,cmd,p],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=2)
   if r.returncode<0: raise RuntimeError(f'{cmd} crashed at {i}')
print(f'fuzz smoke passed: {count} inputs')
