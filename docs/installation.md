# Installation and uninstall

Build, test, and install into a selected prefix:

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build-release
ctest --test-dir build-release --output-on-failure
sudo cmake --install build-release
```

Create a staged package with `cmake --install build-release --prefix "$PWD/stage"`, then archive `stage`. To uninstall, inspect `build-release/install_manifest.txt`, then run `sudo xargs rm -f < build-release/install_manifest.txt` only for a build tree you created and verified.
