# Compiler target configuration

HSL source remains architecture-neutral. Native target selection is controlled only by the HSL compiler environment.

- `HSL_CXX`: C++ compiler driver, default `clang++`.
- `HSL_ARCH_PROFILE`: `x86_64-portable`, `x86_64-v2`, `aarch64-portable`, `armv7-portable`, or `native`.
- `HSL_TARGET`: Clang-compatible target triple, such as `aarch64-linux-gnu`.
- `HSL_CPU`: target CPU passed through `-mcpu`.
- `HSL_SYSROOT`: target sysroot passed through `--sysroot`.

Use `x86_64-portable` for broad Intel and AMD compatibility, including older Ryzen processors. Use `native` only for binaries that will remain on the build machine. Cross-compiling requires a matching target toolchain and sysroot.
