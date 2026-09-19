# Kernel ABI contract

The C++ backend maps HSL fixed-width integers to the matching `<cstdint>` types. `isize` maps to `std::intptr_t`, `usize` to `std::uintptr_t`, and 128-bit values use the compiler's native 128-bit integer extension.

Every ABI-facing structure must be checked with target-side `sizeof`, `alignof`, symbol and section inspection. HSL source must not assume packed structure layout unless a native ABI shim provides and verifies it.

The freestanding runtime contract exposes panic, allocation and volatile MMIO functions. Kernel projects supply these functions. Early boot code may leave allocation unavailable and must avoid allocation-dependent HSL features.
