# HSL 2.0 migration and release notes

HSL 2.0 preserves the core 1.0 syntax while adding enums, payload variants, exhaustive matching, nested generic types, standard generic sum types, maps, sets, system APIs, portable target profiles, stable arena infrastructure, and stronger ownership flow analysis.

## Recommended portable build

```sh
HSL_ARCH_PROFILE=x86_64-portable hsl build program.hsl
```

This baseline is intended for broad Intel and AMD compatibility, including older Ryzen processors. Use `native` only for executables that stay on the build computer.

## Generic values

```hsl
let answer: Option<i64> = Option<i64>.Some(42);
let result: Result<i64, str> = Result<i64, str>.Ok(42);
var values: Map<str, i64> = Map<str, i64>();
var unique: Set<i64> = Set<i64>();
```

## System facilities

```hsl
let path: str = path_join(current_directory(), "value.txt");
print(fs_write(path, "HSL 2.0"));
print(env_get("PATH").is_some());
```
