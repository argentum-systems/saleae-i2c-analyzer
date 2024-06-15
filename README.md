# Build and Install

See: [`i2c-analyzer`](https://github.com/saleae/i2c-analyzer)

Go to: Edit -> Settings -> Custom Low Level Analyzers

## Linux

```bash
cmake -S . -B build
make -C build
```

See: `./build/Analyzers/*.so`

## Windows

- Install Visual Studio 2019 Community
- Install CMake
- Install git

```cmd.exe
cmake -S . -B build
cmake --build build --config Release
```

See: `./build/Analyzers/Debug/*.dll`
