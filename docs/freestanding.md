# Freestanding development

HSL 9.0.0 adds the integer-width and target-width type foundation needed by kernels, firmware tools, boot-time components, drivers and low-level runtimes.

## Supported model

HSL uses a deliberately hybrid freestanding model. Safe kernel policy and algorithms are written in HSL. Raw hardware operations remain in small, auditable C or assembly shims connected through a project-owned ABI. This avoids presenting unchecked hardware access as safe HSL.

Freestanding projects should use compiler object emission, a target profile, a project-owned linker script, and the contracts in `runtime/freestanding/hsl_kernel.h`. Hosted filesystem, process, thread and console APIs must not be called by kernel code.

## Integer types

HSL 9 provides `i8`, `u8`, `i16`, `u16`, `i32`, `u32`, `i64`, `u64`, `i128`, `u128`, `isize` and `usize`. Use explicit-width types for registers and ABI structures. Use `isize` and `usize` for pointer-sized arithmetic.

## Required native flags

The native shim layer should be compiled with `-ffreestanding`, `-fno-exceptions`, `-fno-rtti`, stack-protector and red-zone settings appropriate to the target, and no implicit startup objects. The final binary must be linked with a reviewed linker script.

## Safety boundary

HSL retains non-null ordinary variables and safe references. MMIO, raw addresses, interrupt entry, privileged instructions, packed layouts and inline assembly stay behind reviewed native functions. Each unsafe shim must define its alignment, ordering, lifetime and ownership contract.
