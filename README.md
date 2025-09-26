# Raytracing Engine

A high-performance CPU-based raytracing engine that generates photorealistic images using advanced rendering techniques including reflection, refraction, and material simulation.

## Features

- **Path Tracing**: Implements Monte Carlo path tracing for realistic light simulation
- **Material System**: Supports multiple material types:
  - Lambertian (Diffuse surfaces)
  - Metal (Reflective surfaces with controllable fuzziness)
  - Dielectric (Glass-like materials with refraction)
- **Camera System**: Configurable camera with depth of field simulation
- **Anti-aliasing**: Multi-sample anti-aliasing for smooth edges
- **Scene Generation**: Procedural scene generation with randomized spheres

## Build Requirements

### Prerequisites
- **CMake** 3.10 or higher
- **C++11** compatible compiler:
  - GCC 4.8+ (Linux)
  - Clang 6.0+ (macOS/Linux)
  - Visual Studio 2013+ (Windows)

### Platform-Specific Dependencies

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake g++
```

#### Linux (CentOS/RHEL/Fedora)
```bash
# CentOS/RHEL
sudo yum install gcc-c++ cmake make

# Fedora
sudo dnf install gcc-c++ cmake make
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake via Homebrew (optional)
brew install cmake
```

#### Windows
- Install [Visual Studio 2019/2022](https://visualstudio.microsoft.com/) with C++ development tools
- Install [CMake](https://cmake.org/download/) (3.10+)

## Building the Project

### Quick Start (Linux/macOS)
```bash
# Clone the repository
git clone https://github.com/IcedLeon/Raytracing.git
cd Raytracing

# Build with CMake
mkdir build
cd build
cmake ..
make -j$(nproc)

# Run the raytracer
./raytracing
```

### Detailed Build Instructions

#### 1. Configure the Build
```bash
mkdir build && cd build
cmake .. [OPTIONS]
```

**CMake Configuration Options:**
- `-DCMAKE_BUILD_TYPE=Release` (default) - Optimized build
- `-DCMAKE_BUILD_TYPE=Debug` - Debug build with symbols
- `-DCMAKE_INSTALL_PREFIX=/path/to/install` - Installation directory

#### 2. Compile
```bash
# Linux/macOS
make -j$(nproc)

# Windows (Visual Studio)
cmake --build . --config Release
```

#### 3. Install (Optional)
```bash
make install
# or
cmake --install .
```

### Windows-Specific Build

#### Using Visual Studio Command Prompt
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

#### Using MinGW-w64
```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## Usage

### Basic Usage
```bash
./raytracing
```

The program will generate `output.ppm` in the current directory containing a rendered scene.

### Output Format
The raytracer generates images in PPM (Portable Pixmap) format, which can be:
- Viewed with image viewers that support PPM
- Converted to common formats using ImageMagick:
  ```bash
  convert output.ppm output.png
  ```

### Performance Considerations
- **Default resolution**: 1200×800 pixels
- **Sample count**: 10 samples per pixel (configurable in source)
- **Render time**: Varies from minutes to hours depending on scene complexity and hardware

## Architecture Overview

### Core Components

```
├── main.cpp                 # Application entry point and scene setup
├── appsrc/
│   ├── include/Math/        # Header files
│   │   ├── vec3.h          # 3D vector mathematics
│   │   ├── ray.h           # Ray representation
│   │   ├── sphere.h        # Sphere primitive
│   │   ├── hittable.h      # Base hittable interface
│   │   ├── hittablelist.h  # Collection of hittables
│   │   ├── camera.h        # Camera system
│   │   └── material.h      # Material definitions
│   └── src/Math/           # Implementation files
│       ├── vec3.cpp
│       ├── ray.cpp
│       ├── sphere.cpp
│       ├── hittablelist.cpp
│       └── camera.cpp
```

### Key Classes

- **Vec3**: 3D vector operations (position, direction, color)
- **Ray**: Ray representation with origin and direction
- **Sphere**: Sphere geometric primitive with material properties
- **HittableList**: Collection manager for scene objects
- **Camera**: Virtual camera with configurable parameters
- **Material**: Abstract base for surface material properties

## Current Limitations

- **CPU-only rendering**: No GPU acceleration
- **Limited primitives**: Only spheres supported
- **Fixed scene**: Scene is procedurally generated, not loaded from files
- **No real-time preview**: Offline rendering only

## Future Roadmap

### Planned Improvements

1. **Performance Optimization**
   - SIMD acceleration (SSE/AVX on x86, NEON on ARM)
   - Multi-threading support
   - GPU acceleration with OpenCL/CUDA

2. **Architecture Modernization**
   - Entity Component System (ECS) using [entt](https://github.com/skypjack/entt)
   - Modern C++ features (C++17/20)
   - Plugin architecture for extensibility

3. **Rendering Features**
   - Additional primitives (triangles, planes, meshes)
   - Texture mapping and normal mapping
   - Advanced lighting models (PBR)
   - Volume rendering
   - Interactive preview using [raylib](https://github.com/raysan5/raylib)

4. **User Experience**
   - Scene file format (JSON/XML)
   - Real-time parameter adjustment
   - Progressive rendering
   - GUI interface

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is open source. Please check the repository for license information.

## Acknowledgments

- Based on ["Ray Tracing in One Weekend"](https://raytracing.github.io/) series
- Inspired by modern rendering techniques and research

---

**Note**: This project is currently in transition from QMake to CMake build system as part of modernization efforts. The old `.pro` files are deprecated and will be removed in future versions.