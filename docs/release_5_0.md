# HSL 5.0.0

HSL 5.0 expands native program integration while preserving source compatibility with HSL 4 programs.

## Program arguments

- `arg_count() -> i64` returns the native argument count, including the executable path.
- `arg(index: i64) -> str` returns an argument with checked indexing.
- Generated native entry points now receive and retain `argc` and `argv`.

## Environment and process context

- `env_set(name: str, value: str) -> bool` sets an environment variable.
- `env_unset(name: str) -> bool` removes an environment variable.
- `set_current_directory(path: str) -> bool` changes the process working directory without throwing across the HSL boundary.

## UTF-8

- `utf8_length(value: str) -> i64` counts Unicode scalar positions rather than bytes.
- `utf8_scalar_at(value: str, index: i64) -> i64` returns a checked Unicode scalar value.
- Invalid UTF-8 and invalid scalar indices produce explicit runtime errors.

## Compatibility

The production compiler remains C++20. HSL 7.0 remains reserved for the compiler implemented in HSL.
