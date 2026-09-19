# HSL 8 support policy

HSL 8 is the maintenance line.

## Accepted changes

- Security fixes
- Correctness fixes
- Diagnostics improvements that do not change valid program meaning
- Compatibility fixes for supported operating systems and C++20 toolchains
- Performance fixes that preserve observable behavior
- Documentation and test corrections

## Compatibility

Valid HSL 8.0 programs should remain source compatible throughout HSL 8. Patch releases must not intentionally change language semantics. Any unavoidable compatibility change must be documented before release.

## Toolchain baseline

Building HSL requires CMake 3.20 or newer and a conforming C++20 compiler. GCC and Clang are release-gate toolchains on Linux.

## Defect policy

No project can prove that it contains no defects. A release is considered ready when all automated gates pass and no release-blocking defect is known. Newly discovered security and data-loss defects take priority over all other work.
