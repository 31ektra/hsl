# HSL completion status

HSL 9.0.0 is the long-term-maintenance baseline for the implemented HSL language and toolchain.

The native C++20 compiler has release, sanitizer, adversarial, fuzz-smoke, repository-audit, ownership, nested-list, string, module, deterministic code-generation, object, assembly, installation, and staged compiler-model gates.

The `self_hosted` directory is a deterministic compiler-model foundation written in HSL. It is not a complete production compiler and is not described as a true three-stage bootstrap. The trusted production compiler remains the C++20 implementation.

No software project can prove the absence of all defects. HSL 8 is considered ready when all release gates pass and no release-blocking defect is known.
