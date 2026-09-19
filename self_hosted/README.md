# HSL self-hosted compiler

`compiler.hsl` is the HSL 7 stage-one compiler core. It is compiled by the C++20 stage-zero compiler and provides a deterministic source scanner and compiler manifest.

The stage-zero compiler remains in the repository as the trusted bootstrap implementation. The bootstrap gate builds the HSL compiler core twice, compares generated C++, compares native program output, and checks object and assembly emission.

The HSL implementation currently owns deterministic source ingestion, lexical class accounting, line accounting, and source fingerprinting. Parsing, semantic analysis, ownership analysis, and native code generation remain available through stage zero while their HSL replacements are expanded.
