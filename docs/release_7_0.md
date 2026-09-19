# HSL 7.0.0

HSL 7 introduces the first compiler core written in HSL and a staged bootstrap validation pipeline.

## Self-hosted core

- `self_hosted/compiler.hsl` is compiled by the trusted C++20 stage-zero compiler.
- The HSL core provides deterministic source ingestion, lexical-class accounting, line accounting, and source fingerprinting.
- It exposes a stable stage manifest used by reproducibility tests.

## Bootstrap validation

- The stage-one HSL compiler core is built twice.
- Generated C++ from both builds must be byte-identical.
- Stage-one runtime manifests must be byte-identical.
- Native object and assembly emission are validated for the HSL compiler core.
- The C++20 compiler remains the trusted stage-zero bootstrap implementation.

## Additional hardening

- Bootstrap inputs are isolated in a temporary directory.
- Generated sources, native output, object files, and assembly files are checked independently.
- HSL 6 deterministic-generation and native-artifact gates remain enabled.

## Status

HSL 7 begins the compiler migration to HSL. The lexer/scanner core is implemented in HSL. Parser, semantic, ownership, and code-generation migration continues through HSL 7.x while stage zero remains available for trusted bootstrap and compatibility.
