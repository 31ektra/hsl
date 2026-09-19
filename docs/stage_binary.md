# Staged compiler

`stage/bin/hsl` is the bootstrap compiler used by the staged validation process.

- HSL version: 9.0.0
- Target environment: 64-bit Linux
- SHA-256: `ff308bb9f15b6e60c50bd4051676ed95ca860f281b33d2e15fda388fea0c8276`

The binary is validated by `tools/bootstrap_8.sh` and the release gate. Rebuilds must be produced from reviewed source, checked with both GNU and Clang builds, and compared through the deterministic and bootstrap checks before this checksum is updated.
