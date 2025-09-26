# Build Instructions

This document provides detailed build instructions for the Raytracing Engine across different platforms and build systems.

## Build System Migration

The project has been migrated from **QMake** to **CMake** for better cross-platform support and modern C++ development practices.

### Migration Status
- ✅ **CMake Build System**: Fully implemented and tested
- ✅ **Cross-platform Support**: Windows, Linux, macOS
- ❌ **QMake Support**: Deprecated (files kept for reference)

## Prerequisites

### Required Tools
| Platform | Compiler | CMake | Build Tool |
|----------|----------|-------|------------|
| Linux | GCC 4.8+ / Clang 6.0+ | 3.10+ | make |
| macOS | Clang (Xcode) | 3.10+ | make |
| Windows | MSVC 2013+ / MinGW | 3.10+ | MSBuild / make |

### Installing Prerequisites

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake git
```

#### CentOS 7/8
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake3 git
ln -s /usr/bin/cmake3 /usr/local/bin/cmake
```

#### Fedora
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake (via Homebrew - optional)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake
```

#### Windows
1. **Visual Studio**: Download from [Microsoft](https://visualstudio.microsoft.com/downloads/)
   - Select "Desktop development with C++" workload
   - Includes MSVC compiler and MSBuild

2. **CMake**: Download from [cmake.org](https://cmake.org/download/)
   - Choose "Add CMake to system PATH" during installation

3. **Git**: Download from [git-scm.com](https://git-scm.com/download/win)

## Build Process

### Standard Build (Recommended)

```bash
# 1. Clone the repository
git clone https://github.com/IcedLeon/Raytracing.git
cd Raytracing

# 2. Create build directory
mkdir build
cd build

# 3. Configure build
cmake ..

# 4. Build
cmake --build . --config Release

# 5. Run
./raytracing  # Linux/macOS
raytracing.exe  # Windows
```

### Advanced Build Options

#### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

#### Custom Install Location
```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/raytracing ..
cmake --build .
cmake --install .
```

#### Parallel Build
```bash
# Linux/macOS - use all CPU cores
make -j$(nproc)

# Windows - use all CPU cores
cmake --build . -- /m
```

## Platform-Specific Instructions

### Linux

#### Standard GCC Build
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=g++ ..
make -j$(nproc)
```

#### Clang Build
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++ ..
make -j$(nproc)
```

#### Profile-Guided Optimization (Advanced)
```bash
# First build with profiling
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make -j$(nproc)
./raytracing  # Generate profile data

# Rebuild with optimizations
make clean
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ..
make -j$(nproc)
```

### macOS

#### Universal Binary (Intel + Apple Silicon)
```bash
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
make -j$(sysctl -n hw.logicalcpu)
```

#### Minimum macOS Version
```bash
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 ..
```

### Windows

#### Visual Studio 2019/2022
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release --parallel
```

#### MinGW-w64
```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j%NUMBER_OF_PROCESSORS%
```

#### Ninja Build System (Faster)
```cmd
# Install Ninja first: https://ninja-build.org/
mkdir build
cd build
cmake .. -G Ninja
ninja
```

## Build Configurations

### Release (Default)
- **Optimization**: High (-O3/O2)
- **Debug Info**: None
- **Assertions**: Disabled
- **Use Case**: Production builds, performance testing

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Debug
- **Optimization**: None (-O0)
- **Debug Info**: Full (-g)
- **Assertions**: Enabled
- **Use Case**: Development, debugging

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### RelWithDebInfo
- **Optimization**: High with debug info
- **Use Case**: Performance profiling

```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

## Troubleshooting

### Common Issues

#### CMake Version Too Old
```
Error: CMake 3.10 or higher is required
```
**Solution**: Update CMake or use system package manager

#### Compiler Not Found
```
Error: Could not find a suitable compiler
```
**Solution**: Install build tools or set compiler path:
```bash
cmake -DCMAKE_CXX_COMPILER=/path/to/compiler ..
```

#### Missing Headers
```
Error: fatal error: 'iostream' file not found
```
**Solution**: Install development headers:
```bash
# Ubuntu/Debian
sudo apt install libc6-dev

# CentOS/RHEL
sudo yum install glibc-devel
```

#### Windows Path Issues
```
Error: The filename or extension is too long
```
**Solution**: Use shorter build path or enable long paths in Windows

#### Out of Memory During Build
**Solution**: Reduce parallel jobs:
```bash
make -j2  # Use only 2 cores instead of all
```

### Performance Issues

#### Slow Rendering
- **Check build type**: Ensure Release build is used
- **Compiler optimization**: Verify -O3 flags are applied
- **System resources**: Monitor CPU/memory usage

#### Build Performance
- **Use Ninja**: Faster than Make on some systems
- **Parallel builds**: Use all available CPU cores
- **SSD storage**: Faster than HDD for builds

## Verification

### Build Verification
```bash
# Check executable exists
ls -la raytracing  # Linux/macOS
dir raytracing.exe  # Windows

# Quick smoke test (5 second timeout)
timeout 5s ./raytracing  # Linux/macOS
```

### Expected Output
- **Build time**: 30-60 seconds (depending on system)
- **Executable size**: ~100-500KB (varies by platform/compiler)
- **Runtime**: Several minutes for full render

## Integration with IDEs

### Visual Studio Code
1. Install C/C++ extension
2. Install CMake Tools extension
3. Open project folder
4. Use Ctrl+Shift+P → "CMake: Configure"

### Visual Studio (Windows)
1. File → Open → CMake
2. Select CMakeLists.txt
3. Build → Build All

### CLion
1. File → Open → Select project directory
2. CLion automatically detects CMake project
3. Build → Build Project

## Continuous Integration

### GitHub Actions Example
```yaml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        make -j$(nproc)
```

## Next Steps

After successful build:
1. **Run the application**: Generate your first rendered image
2. **Experiment**: Modify scene parameters in main.cpp
3. **Performance tuning**: Try different compiler flags
4. **Contributing**: Check CONTRIBUTING.md for development guidelines