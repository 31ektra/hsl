# HSL 9.0.0

HSL 9.0.0 is the long-term-maintenance baseline for the native programming language of Helios.

## Release guarantees

- Existing HSL 7 source accepted by the compiler remains supported.
- The compiler, runtime support, object emission, assembly emission, modules, ownership analysis, matching, collections, and system APIs remain covered by automated regression tests.
- Generated C++ is checked for deterministic output.
- The staged HSL compiler model is rebuilt twice and compared byte for byte.
- GCC and Clang release builds are supported.
- AddressSanitizer and UndefinedBehaviorSanitizer builds are supported.
- Repository auditing, fuzz smoke testing, installation testing, and source-archive reconstruction are part of the release gate.
- HSL does not introduce a null-variable design.

## Bootstrap status

The trusted production compiler remains the C++20 implementation. The HSL implementation in `self_hosted` is a deterministic stage-one compiler model and validation corpus. It is not falsely represented as a complete self-hosted production compiler.

## Maintenance policy

HSL 8 receives compatibility, correctness, security, and unavoidable platform adaptations. New language redesigns are outside the planned maintenance scope. See `docs/support_policy.md`.
