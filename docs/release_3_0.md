# HSL 3.0.0

HSL 3.0 strengthens the native compiler without replacing it with a self-hosted implementation.

## Changes

- Match-arm ownership state merging
- Structured shell-free process launch with `process_run_program`
- Directory creation with `fs_create_directory` and `fs_create_directories`
- File removal and renaming
- File and directory classification
- Path parent, filename, extension, and lexical normalization
- Existing generic collections, enum matching, portable targets, and arena infrastructure retained

## Runtime APIs

```hsl
fs_create_directory(path: str) -> bool
fs_create_directories(path: str) -> bool
fs_remove(path: str) -> bool
fs_rename(from: str, to: str) -> bool
fs_is_file(path: str) -> bool
fs_is_directory(path: str) -> bool
path_parent(path: str) -> str
path_filename(path: str) -> str
path_extension(path: str) -> str
path_normalize(path: str) -> str
process_run_program(executable: str) -> i64
```

`process_run_program` launches the executable directly and does not invoke a command shell.

## Compiler implementation

The production compiler remains C++20 by explicit project choice. The self-hosted model is retained as a language-validation fixture and is not represented as a complete compiler.
