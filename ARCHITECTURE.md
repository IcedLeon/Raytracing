# Architecture Documentation

This document describes the current architecture of the Raytracing Engine and the planned modernization roadmap.

## Current Architecture (Legacy)

### Overview
The current implementation follows a procedural approach with object-oriented components for mathematical operations and scene representation.

### Component Structure

```
Raytracing Engine
├── Math Foundation
│   ├── Vec3 (3D Vectors)
│   ├── Ray (Ray representation)
│   └── Mathematical operations
├── Geometry
│   ├── Hittable (Base interface)
│   ├── Sphere (Primary primitive)
│   └── HittableList (Scene container)
├── Rendering
│   ├── Camera (View management)
│   ├── Material System
│   └── Color/Lighting calculations
└── Scene Generation
    ├── Procedural scene creation
    └── PPM file output
```

### Class Hierarchy

```cpp
// Base interfaces
class Hittable {
    virtual bool Hit(Ray, float, float, HitRecord&) = 0;
};

class Material {
    virtual bool Scatter(Ray, HitRecord, Vec3&, Ray&) = 0;
};

// Implementations
class Sphere : public Hittable { ... };
class HittableList : public Hittable { ... };

class Lambertian : public Material { ... };
class Metal : public Material { ... };
class Dielectric : public Material { ... };
```

### Data Flow

```
User Input → Scene Setup → Ray Generation → 
Hit Testing → Material Interaction → Color Calculation → 
Anti-aliasing → Gamma Correction → PPM Output
```

## Current Implementation Details

### Core Mathematics (`appsrc/include/Math/`)

#### Vec3 Class
```cpp
class Vec3 {
    float e[3];  // x, y, z components
    
    // Basic operations
    Vec3 operator+(const Vec3&) const;
    Vec3 operator*(float) const;
    float Length() const;
    // ... more operations
};
```

**Responsibilities:**
- 3D vector arithmetic
- Color representation (R, G, B)
- Position and direction vectors

#### Ray Class
```cpp
class Ray {
    Vec3 A, B;  // Origin and direction
    
public:
    Vec3 Origin() const { return A; }
    Vec3 Direction() const { return B; }
    Vec3 PointAtParamenter(float t) const { return A + t*B; }
};
```

**Responsibilities:**
- Ray representation for tracing
- Point-on-ray calculations

### Geometry System

#### Hittable Interface
```cpp
struct HitRecord {
    float t;              // Ray parameter
    Vec3 p;              // Hit point
    Vec3 normal;         // Surface normal
    Material* mat_ptr;   // Material properties
};

class Hittable {
public:
    virtual bool Hit(const Ray& r, float t_min, float t_max, HitRecord& rec) = 0;
};
```

#### Sphere Implementation
```cpp
class Sphere : public Hittable {
    Vec3 center;
    float radius;
    Material* material;
    
public:
    bool Hit(const Ray& r, float t_min, float t_max, HitRecord& rec) override;
};
```

**Ray-Sphere Intersection:**
- Solves quadratic equation: `t²(d·d) + 2t(d·(o-c)) + (o-c)·(o-c) - r² = 0`
- Returns closest valid intersection within [t_min, t_max]

### Material System

#### Lambertian (Diffuse)
```cpp
class Lambertian : public Material {
    Vec3 albedo;  // Surface color
    
    bool Scatter(const Ray& r_in, const HitRecord& rec, 
                Vec3& attenuation, Ray& scattered) override {
        Vec3 target = rec.p + rec.normal + random_in_unit_sphere();
        scattered = Ray(rec.p, target - rec.p);
        attenuation = albedo;
        return true;
    }
};
```

#### Metal (Reflective)
```cpp
class Metal : public Material {
    Vec3 albedo;
    float fuzz;  // Roughness factor
    
    bool Scatter(...) override {
        Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = Ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        attenuation = albedo;
        return dot(scattered.direction(), rec.normal) > 0;
    }
};
```

#### Dielectric (Glass)
```cpp
class Dielectric : public Material {
    float ref_idx;  // Refractive index
    
    bool Scatter(...) override {
        // Handles both reflection and refraction
        // Uses Schlick approximation for Fresnel reflectance
        // Probabilistically chooses between reflection/refraction
    }
};
```

### Camera System

```cpp
class Camera {
    Vec3 origin;
    Vec3 lower_left_corner;
    Vec3 horizontal, vertical;
    Vec3 u, v, w;  // Camera coordinate system
    float lens_radius;
    
public:
    Ray get_ray(float s, float t) {
        Vec3 rd = lens_radius * random_in_unit_disk();
        Vec3 offset = u * rd.x() + v * rd.y();
        return Ray(origin + offset, 
                  lower_left_corner + s*horizontal + t*vertical - origin - offset);
    }
};
```

**Features:**
- Configurable field of view
- Depth of field simulation
- Arbitrary camera positioning

### Rendering Pipeline

#### Main Rendering Loop
```cpp
for (int j = ny-1; j >= 0; j--) {
    for (int i = 0; i < nx; i++) {
        Vec3 col(0, 0, 0);
        
        // Anti-aliasing: multiple samples per pixel
        for (int s = 0; s < ns; s++) {
            float u = float(i + random()) / float(nx);
            float v = float(j + random()) / float(ny);
            Ray r = camera.get_ray(u, v);
            col += color(r, world, 0);  // Recursive ray tracing
        }
        
        col /= float(ns);  // Average samples
        col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));  // Gamma correction
        
        // Convert to RGB and output
        output_pixel(col);
    }
}
```

#### Recursive Color Calculation
```cpp
Vec3 color(const Ray& r, Hittable* world, int depth) {
    HitRecord rec;
    
    if (world->hit(r, 0.001, MAXFLOAT, rec)) {
        Ray scattered;
        Vec3 attenuation;
        
        if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
            return attenuation * color(scattered, world, depth+1);  // Recursive
        } else {
            return Vec3(0, 0, 0);  // Ray absorption
        }
    } else {
        // Sky gradient
        Vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0-t)*Vec3(1.0, 1.0, 1.0) + t*Vec3(0.5, 0.7, 1.0);
    }
}
```

## Current Limitations

### Performance Issues
1. **Single-threaded**: No parallelization of pixel calculations
2. **No SIMD**: Vector operations not optimized
3. **Memory allocation**: Dynamic allocation in hot paths
4. **Cache inefficiency**: Poor data locality for large scenes

### Architectural Issues
1. **Tight coupling**: Hard to extend with new features
2. **Global state**: Scene setup mixed with rendering logic
3. **Fixed pipeline**: No plugin system for custom materials/geometry
4. **Limited scalability**: Not suitable for complex scenes

### Maintenance Issues
1. **Mixed responsibilities**: Single files handling multiple concerns
2. **No unit tests**: Difficult to refactor safely
3. **Platform dependencies**: Some code paths not cross-platform
4. **Legacy patterns**: C-style casts, raw pointers

## Modernization Roadmap

### Phase 1: Foundation (Current)
- [x] **CMake Migration**: Modern build system
- [x] **Code cleanup**: Fix warnings, remove QMake dependency
- [x] **Documentation**: Comprehensive build and usage guides
- [ ] **Basic refactoring**: Separate concerns, improve modularity

### Phase 2: Performance Optimization

#### SIMD Integration
**Recommended Libraries:**
```cpp
// Cross-platform SIMD
#include <immintrin.h>  // x86 SSE/AVX
#include <arm_neon.h>   // ARM NEON

// Or use abstraction library:
#include <glm/glm.hpp>              // GLM with SIMD
#include <Eigen/Dense>              // Eigen with vectorization
#include <DirectXMath.h>            // Windows DirectXMath
```

**Target Optimizations:**
- Vector operations (Vec3 arithmetic)
- Ray-sphere intersection batching
- Color blending operations

#### Multi-threading
```cpp
// OpenMP integration
#pragma omp parallel for
for (int j = 0; j < height; j++) {
    // Pixel row processing
}

// Or std::thread pool
std::vector<std::thread> workers;
for (int i = 0; i < num_threads; i++) {
    workers.emplace_back(render_tile, tile_bounds[i]);
}
```

### Phase 3: Architecture Modernization

#### Entity Component System (ECS)
**Using [entt](https://github.com/skypjack/entt):**

```cpp
#include <entt/entt.hpp>

// Components
struct Position { float x, y, z; };
struct Radius { float value; };
struct Material { MaterialType type; Vec3 albedo; };

// System
class RenderSystem {
    entt::registry registry;
    
public:
    void render() {
        auto view = registry.view<Position, Radius, Material>();
        
        for (auto entity : view) {
            auto& pos = view.get<Position>(entity);
            auto& rad = view.get<Radius>(entity);
            auto& mat = view.get<Material>(entity);
            
            // Process entity for rendering
        }
    }
};
```

**Benefits:**
- Better data locality
- Easier to add new component types
- Natural parallelization boundaries
- Flexible scene composition

#### Modern C++ Features
```cpp
// C++17/20 features
#include <memory>
#include <optional>
#include <variant>
#include <execution>

// Smart pointers instead of raw pointers
std::unique_ptr<Material> material;
std::shared_ptr<Texture> texture;

// Variants for material types
using MaterialVariant = std::variant<Lambertian, Metal, Dielectric>;

// Parallel algorithms
std::for_each(std::execution::par_unseq, 
              pixels.begin(), pixels.end(),
              [](auto& pixel) { /* render pixel */ });
```

### Phase 4: GPU Integration

#### Raylib Integration
```cpp
#include <raylib.h>

class RaylibRenderer {
    RenderTexture2D target;
    
public:
    void initialize() {
        InitWindow(800, 600, "Raytracing Engine");
        target = LoadRenderTexture(800, 600);
    }
    
    void update_frame(const std::vector<Color>& pixels) {
        BeginTextureMode(target);
        // Update texture with raytraced pixels
        EndTextureMode();
        
        BeginDrawing();
        DrawTexture(target.texture, 0, 0, WHITE);
        EndDrawing();
    }
};
```

#### Compute Shader Integration
```cpp
// OpenGL compute shader for GPU raytracing
const char* compute_shader = R"(
#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    
    // Ray generation and tracing on GPU
    vec3 color = raytrace(pixel_coords);
    
    imageStore(img_output, pixel_coords, vec4(color, 1.0));
}
)";
```

### Phase 5: Advanced Features

#### Scene Loading
```cpp
// JSON scene format
{
    "camera": {
        "position": [13, 2, 3],
        "look_at": [0, 0, 0],
        "fov": 20
    },
    "objects": [
        {
            "type": "sphere",
            "center": [0, 0, -1],
            "radius": 0.5,
            "material": {
                "type": "lambertian",
                "albedo": [0.1, 0.2, 0.5]
            }
        }
    ]
}
```

#### Plugin Architecture
```cpp
class MaterialPlugin {
public:
    virtual ~MaterialPlugin() = default;
    virtual std::unique_ptr<Material> create(const json& params) = 0;
    virtual std::string name() const = 0;
};

class PluginManager {
    std::map<std::string, std::unique_ptr<MaterialPlugin>> plugins;
    
public:
    void register_plugin(std::unique_ptr<MaterialPlugin> plugin) {
        plugins[plugin->name()] = std::move(plugin);
    }
    
    std::unique_ptr<Material> create_material(const std::string& type, const json& params) {
        return plugins[type]->create(params);
    }
};
```

## Target Architecture (Future)

```
Modern Raytracing Engine
├── Core Framework (ECS-based)
│   ├── Entity Manager (entt)
│   ├── Component Systems
│   └── Resource Management
├── Math Library (SIMD-optimized)
│   ├── Vector Operations
│   ├── Matrix Operations
│   └── Intersection Algorithms
├── Rendering Pipeline
│   ├── CPU Path Tracer (Multi-threaded)
│   ├── GPU Compute Shaders
│   └── Hybrid CPU/GPU Pipeline
├── Scene Management
│   ├── Scene Loader (JSON/XML)
│   ├── Asset Pipeline
│   └── Streaming System
├── Material System (Plugin-based)
│   ├── PBR Materials
│   ├── Custom Shaders
│   └── Texture Support
├── User Interface (raylib)
│   ├── Real-time Preview
│   ├── Parameter Adjustment
│   └── Progress Monitoring
└── Export System
    ├── Multiple Formats (PNG, EXR, HDR)
    ├── Animation Support
    └── Render Queue Management
```

This architecture will provide:
- **Performance**: 10-100x faster rendering through GPU acceleration and SIMD
- **Flexibility**: Easy to extend with new materials, geometry, and effects
- **Usability**: Real-time preview and interactive parameter adjustment
- **Scalability**: Handle complex scenes with millions of primitives
- **Maintainability**: Clean separation of concerns and comprehensive testing