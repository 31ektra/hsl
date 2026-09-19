# HSL 9.0.0

HSL 9.0.0 establishes the operating-system development foundation while preserving the HSL 8 hosted language.

Highlights:

- fixed-width 8, 16, 32, 64 and 128-bit signed and unsigned integers;
- pointer-width `isize` and `usize` integers;
- `never` type recognition for non-returning interfaces;
- native C++ mappings for every new primitive type;
- kernel ABI and freestanding development contracts;
- volatile MMIO runtime interface for native shims;
- regression tests for kernel-width type parsing and code generation;
- no null-variable semantics.
