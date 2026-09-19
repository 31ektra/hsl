# HSL 6.0.0

HSL 6.0 is the bootstrap-readiness release of the C++20 production compiler.

## Native artifact emission

- `hsl emit-object <file>` emits a relocatable native object file beside the source.
- `hsl emit-assembly <file>` emits target assembly beside the source.
- Both commands honor the existing `HSL_CXX`, `HSL_TARGET`, `HSL_SYSROOT`, `HSL_CPU`, and `HSL_PROFILE` configuration.
- Temporary generated C++ is removed after artifact emission.

## Reproducibility

- `hsl verify-determinism <file>` compiles the source tree twice in memory and fails if generated C++ differs.
- The command provides a focused reproducibility gate for bootstrap and package pipelines.

## Bootstrap position

HSL 6 retains the C++20 compiler as the production stage-zero implementation. HSL 7.0 remains reserved for the production compiler implemented in HSL and validated through staged compiler equivalence.
