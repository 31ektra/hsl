# HSL 4.0.0

HSL 4.0 is a native-toolchain and runtime expansion. The production compiler remains C++20.

## Compiler interface

- `hsl emit-cpp <file>` writes generated C++ to standard output.
- `hsl build --keep-cpp <file>` retains the generated C++ file.
- Existing build, check, AST, token, target, sanitizer, fuzz, and package workflows remain supported.

## Runtime additions

- `process_run_arg(executable, argument)` directly launches a program with one argument without shell interpretation.
- `fs_copy(from, to)` copies a file with replacement.
- `fs_remove_all(path)` recursively removes a path and returns the number of entries removed, or `-1` on error.
- `fs_file_size(path)` returns a file size, or `-1` on error.
- `time_unix_ms()` returns Unix time in milliseconds.
- `sleep_ms(duration)` sleeps for a non-negative millisecond duration.
- `read_line()` reads one line from standard input.
- `i64_to_string(value)` converts an integer to text.
- `try_parse_i64(value)` returns `Option<i64>` without introducing null values.

## Analysis

HSL 4 retains branch and match-arm move-state merging, ownership diagnostics, generic built-in collections, enum payload matching, portable target profiles, and arena infrastructure.

## Compatibility

HSL 4.0 is source-compatible with valid HSL 3.0 programs. The previous shell-based `process_run` remains available for compatibility; new code should prefer direct process APIs.
