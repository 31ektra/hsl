# HSL 8 language reference

The HSL 8 language is a statically typed native language with deterministic compilation, explicit mutability, ownership-aware checking, structures, enums, matching, modules, collections, and system APIs.

## Design invariants

- Variables are never implicitly null.
- Mutation requires `var`; immutable bindings use `let`.
- Conditions have type `bool`.
- Moves are checked before code generation.
- Match expressions are checked for supported exhaustiveness rules.
- Module imports are explicit and deterministic.
- Compiler diagnostics include source locations.

## Canonical syntax

The complete accepted syntax and examples are maintained in `README.md`, while executable behavior is defined by the parser, semantic, type-checker, code-generation, and regression suites. If prose and tested compiler behavior conflict, the behavior is a defect that must be resolved rather than silently documented away.

## Stability

HSL 8 language behavior is frozen for maintenance. Security, correctness, diagnostics, implementation, and platform compatibility work may continue without intentionally expanding the language.
