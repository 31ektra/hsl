# Building HSL 2.0.0

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Install into a user prefix:

```sh
cmake --install build --prefix "$HOME/.local"
```

The generated native-code driver defaults to `clang++`. Set `HSL_CXX` to another compatible C++20 driver. Target settings are documented in `docs/targeting.md`.
