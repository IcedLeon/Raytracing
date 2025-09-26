# Performance Optimization Plan

This document outlines a comprehensive plan to optimize the raytracing engine for maximum performance while maintaining code quality and cross-platform compatibility.

## Current Performance Analysis

### Baseline Metrics (1200×800, 10 samples/pixel)
- **Render Time**: ~3-5 minutes (single-threaded, CPU-only)
- **Memory Usage**: ~50MB peak
- **CPU Utilization**: Single core (12.5% on 8-core system)
- **Cache Miss Rate**: High (poor data locality)

### Performance Bottlenecks

1. **Single-threaded execution** (95% of potential speedup lost)
2. **Scalar math operations** (no SIMD utilization)
3. **Random memory access patterns** (cache inefficient)
4. **Dynamic memory allocation** in hot paths
5. **Recursive ray tracing** (stack overhead)

## Optimization Roadmap

### Phase 1: Foundation & Profiling (Weeks 1-2)

#### 1.1 Performance Measurement Infrastructure
```cpp
// High-resolution timing
class PerformanceTimer {
    std::chrono::high_resolution_clock::time_point start;
    
public:
    void begin() { start = std::chrono::high_resolution_clock::now(); }
    
    double elapsed_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
};

// Performance counters
struct RenderStats {
    size_t rays_traced = 0;
    size_t intersection_tests = 0;
    size_t material_evaluations = 0;
    double total_time_ms = 0;
    double intersection_time_ms = 0;
    double shading_time_ms = 0;
};
```

#### 1.2 Baseline Profiling
```bash
# Profile with perf (Linux)
perf record -g ./raytracing
perf report

# Profile with Instruments (macOS)
instruments -t "Time Profiler" ./raytracing

# Profile with Visual Studio (Windows)
# Use built-in profiler or Intel VTune
```

### Phase 2: Multi-threading (Weeks 3-4)

#### 2.1 Tile-Based Rendering
```cpp
struct RenderTile {
    int x_start, y_start;
    int width, height;
    int samples_per_pixel;
};

class TiledRenderer {
    std::vector<RenderTile> tiles;
    ThreadPool thread_pool;
    
public:
    void render_parallel(const Scene& scene, Image& output) {
        std::vector<std::future<void>> futures;
        
        for (const auto& tile : tiles) {
            futures.push_back(
                thread_pool.enqueue([this, &scene, &output, tile] {
                    render_tile(scene, output, tile);
                })
            );
        }
        
        // Wait for all tiles to complete
        for (auto& future : futures) {
            future.wait();
        }
    }
};
```

#### 2.2 Work-Stealing Thread Pool
```cpp
class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
    
public:
    ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        
                        if (stop && tasks.empty()) return;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    template<class F>
    auto enqueue(F&& f) -> std::future<typename std::result_of<F()>::type> {
        using return_type = typename std::result_of<F()>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<F>(f)
        );
        
        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace([task](){ (*task)(); });
        }
        
        condition.notify_one();
        return res;
    }
};
```

**Expected Speedup**: 4-8x on typical multi-core systems

### Phase 3: SIMD Optimization (Weeks 5-6)

#### 3.1 Cross-Platform SIMD Abstraction

**Option A: Platform-Specific**
```cpp
#ifdef __SSE__
#include <immintrin.h>
using simd_float4 = __m128;
#elif defined(__ARM_NEON)
#include <arm_neon.h>
using simd_float4 = float32x4_t;
#else
struct simd_float4 { float data[4]; };
#endif

class SIMDVec3 {
    simd_float4 data;  // [x, y, z, w] where w is padding
    
public:
    SIMDVec3(float x, float y, float z) {
#ifdef __SSE__
        data = _mm_set_ps(0, z, y, x);
#elif defined(__ARM_NEON)
        data = {x, y, z, 0};
#else
        data = {x, y, z, 0};
#endif
    }
    
    SIMDVec3 operator+(const SIMDVec3& other) const {
        SIMDVec3 result;
#ifdef __SSE__
        result.data = _mm_add_ps(data, other.data);
#elif defined(__ARM_NEON)
        result.data = vaddq_f32(data, other.data);
#else
        result.data.data[0] = data.data[0] + other.data.data[0];
        result.data.data[1] = data.data[1] + other.data.data[1];
        result.data.data[2] = data.data[2] + other.data.data[2];
#endif
        return result;
    }
};
```

**Option B: Library-Based (Recommended)**
```cpp
// Use GLM with SIMD
#define GLM_FORCE_INLINE
#define GLM_FORCE_SIMD_AVX2  // or appropriate SIMD level
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using Vec3 = glm::vec3;
using Mat4 = glm::mat4;

// GLM automatically uses SIMD when available
Vec3 optimized_cross(const Vec3& a, const Vec3& b) {
    return glm::cross(a, b);  // SIMD-optimized internally
}
```

#### 3.2 Vectorized Ray-Sphere Intersection
```cpp
// Process 4 rays simultaneously
struct Ray4 {
    simd_float4 origin_x, origin_y, origin_z;
    simd_float4 dir_x, dir_y, dir_z;
};

struct Sphere4 {
    simd_float4 center_x, center_y, center_z;
    simd_float4 radius_squared;
};

// Returns 4 intersection results at once
simd_float4 intersect_ray4_sphere4(const Ray4& rays, const Sphere4& spheres) {
    // Vectorized quadratic equation solving
    simd_float4 oc_x = _mm_sub_ps(rays.origin_x, spheres.center_x);
    simd_float4 oc_y = _mm_sub_ps(rays.origin_y, spheres.center_y);
    simd_float4 oc_z = _mm_sub_ps(rays.origin_z, spheres.center_z);
    
    simd_float4 a = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(rays.dir_x, rays.dir_x),
            _mm_mul_ps(rays.dir_y, rays.dir_y)
        ),
        _mm_mul_ps(rays.dir_z, rays.dir_z)
    );
    
    // Continue with vectorized quadratic formula...
    // Returns packed t-values for 4 intersections
}
```

**Expected Speedup**: 2-4x for math-heavy operations

### Phase 4: Memory Optimization (Weeks 7-8)

#### 4.1 Structure of Arrays (SoA)
```cpp
// Instead of Array of Structures (AoS)
struct Sphere_AoS {
    Vec3 center;
    float radius;
    Material* material;
};
std::vector<Sphere_AoS> spheres;

// Use Structure of Arrays (SoA) for better cache utilization
struct SpheresSoA {
    std::vector<float> center_x, center_y, center_z;
    std::vector<float> radius;
    std::vector<MaterialID> material_ids;
    
    size_t size() const { return center_x.size(); }
    
    void add_sphere(const Vec3& center, float r, MaterialID mat) {
        center_x.push_back(center.x);
        center_y.push_back(center.y);
        center_z.push_back(center.z);
        radius.push_back(r);
        material_ids.push_back(mat);
    }
};
```

#### 4.2 Memory Pool Allocation
```cpp
class MemoryPool {
    std::vector<uint8_t> buffer;
    size_t offset = 0;
    size_t capacity;
    
public:
    MemoryPool(size_t size) : buffer(size), capacity(size) {}
    
    template<typename T>
    T* allocate(size_t count = 1) {
        size_t size = sizeof(T) * count;
        size_t aligned_size = (size + alignof(T) - 1) & ~(alignof(T) - 1);
        
        if (offset + aligned_size > capacity) {
            throw std::bad_alloc();
        }
        
        T* ptr = reinterpret_cast<T*>(buffer.data() + offset);
        offset += aligned_size;
        return ptr;
    }
    
    void reset() { offset = 0; }
};

// Use per-thread memory pools to avoid allocation overhead
thread_local MemoryPool thread_pool(1024 * 1024);  // 1MB per thread
```

#### 4.3 Cache-Friendly Data Layout
```cpp
// Spatial data structure for efficient ray-scene intersection
class BVH {
    struct Node {
        BoundingBox bounds;
        union {
            struct {  // Internal node
                uint32_t left_child;
                uint32_t right_child;
            };
            struct {  // Leaf node
                uint32_t primitive_offset;
                uint32_t primitive_count;
            };
        };
        bool is_leaf;
    };
    
    std::vector<Node> nodes;
    std::vector<uint32_t> primitive_indices;
    
public:
    bool intersect(const Ray& ray, HitRecord& record) const {
        // Traverse BVH efficiently with good cache locality
        std::stack<uint32_t> node_stack;
        node_stack.push(0);  // Root node
        
        bool hit = false;
        float closest_t = std::numeric_limits<float>::max();
        
        while (!node_stack.empty()) {
            uint32_t node_index = node_stack.top();
            node_stack.pop();
            
            const Node& node = nodes[node_index];
            
            if (!node.bounds.intersect(ray)) continue;
            
            if (node.is_leaf) {
                // Test primitives in leaf
                for (uint32_t i = 0; i < node.primitive_count; ++i) {
                    uint32_t prim_index = primitive_indices[node.primitive_offset + i];
                    // Test intersection with primitive...
                }
            } else {
                // Add children to stack
                node_stack.push(node.left_child);
                node_stack.push(node.right_child);
            }
        }
        
        return hit;
    }
};
```

**Expected Speedup**: 2-3x due to better cache utilization

### Phase 5: Algorithm Optimization (Weeks 9-10)

#### 5.1 Iterative Ray Tracing (Remove Recursion)
```cpp
Vec3 trace_ray_iterative(const Ray& initial_ray, const Scene& scene) {
    Vec3 final_color(0, 0, 0);
    Vec3 throughput(1, 1, 1);  // Accumulated attenuation
    Ray current_ray = initial_ray;
    
    for (int depth = 0; depth < MAX_DEPTH; ++depth) {
        HitRecord record;
        
        if (!scene.intersect(current_ray, record)) {
            // Hit sky
            final_color += throughput * sky_color(current_ray);
            break;
        }
        
        // Evaluate material
        Ray scattered;
        Vec3 attenuation;
        
        if (!record.material->scatter(current_ray, record, attenuation, scattered)) {
            // Ray absorbed
            break;
        }
        
        throughput *= attenuation;
        current_ray = scattered;
        
        // Russian roulette termination for deep rays
        if (depth > 3) {
            float max_component = std::max({throughput.x, throughput.y, throughput.z});
            if (random_float() > max_component) {
                break;
            }
            throughput /= max_component;  // Compensate for termination probability
        }
    }
    
    return final_color;
}
```

#### 5.2 Next Event Estimation (Direct Lighting)
```cpp
Vec3 estimate_direct_lighting(const HitRecord& hit, const Scene& scene) {
    Vec3 direct_light(0, 0, 0);
    
    // Sample all light sources
    for (const auto& light : scene.lights) {
        // Sample light source
        Vec3 light_pos = light.sample_position();
        Vec3 to_light = light_pos - hit.point;
        float distance = to_light.length();
        to_light /= distance;
        
        // Check for occlusion
        Ray shadow_ray(hit.point + hit.normal * 0.001f, to_light);
        if (!scene.intersect_any(shadow_ray, distance)) {
            // Light is visible
            float cos_theta = std::max(0.0f, dot(hit.normal, to_light));
            Vec3 brdf = hit.material->evaluate_brdf(hit.wo, to_light, hit.normal);
            float light_pdf = light.pdf(hit.point, light_pos);
            
            direct_light += brdf * light.radiance * cos_theta / light_pdf;
        }
    }
    
    return direct_light;
}
```

**Expected Speedup**: 2-5x due to more efficient sampling

### Phase 6: GPU Acceleration (Weeks 11-12)

#### 6.1 OpenCL Compute Kernel
```cpp
// OpenCL kernel for parallel ray tracing
const char* raytrace_kernel = R"(
__kernel void raytrace(__global float* output,
                      __global float* spheres,  // SoA layout
                      int num_spheres,
                      int width, int height,
                      int samples) {
    int pixel_id = get_global_id(0);
    int x = pixel_id % width;
    int y = pixel_id / width;
    
    if (x >= width || y >= height) return;
    
    float3 color = (float3)(0, 0, 0);
    
    for (int s = 0; s < samples; s++) {
        // Generate ray for pixel (x, y) with sample s
        float u = (x + random()) / width;
        float v = (y + random()) / height;
        
        // Trace ray and accumulate color
        color += trace_ray_gpu(generate_ray(u, v), spheres, num_spheres);
    }
    
    color /= samples;
    
    // Write to output buffer
    int idx = (y * width + x) * 3;
    output[idx + 0] = sqrt(color.x);  // Gamma correction
    output[idx + 1] = sqrt(color.y);
    output[idx + 2] = sqrt(color.z);
}
)";
```

#### 6.2 CUDA Implementation (NVIDIA GPUs)
```cpp
__global__ void raytrace_cuda(float* output,
                             const Sphere* spheres,
                             int num_spheres,
                             int width, int height,
                             int samples) {
    int pixel_id = blockIdx.x * blockDim.x + threadIdx.x;
    int x = pixel_id % width;
    int y = pixel_id / width;
    
    if (x >= width || y >= height) return;
    
    curandState rand_state;
    curand_init(pixel_id, 0, 0, &rand_state);
    
    float3 color = make_float3(0, 0, 0);
    
    for (int s = 0; s < samples; s++) {
        float u = (x + curand_uniform(&rand_state)) / width;
        float v = (y + curand_uniform(&rand_state)) / height;
        
        color = color + trace_ray_cuda(generate_ray(u, v), spheres, num_spheres, &rand_state);
    }
    
    color = color / samples;
    
    int idx = (y * width + x) * 3;
    output[idx + 0] = sqrtf(color.x);
    output[idx + 1] = sqrtf(color.y);
    output[idx + 2] = sqrtf(color.z);
}
```

**Expected Speedup**: 50-500x on modern GPUs

### Phase 7: Advanced Optimizations (Weeks 13-14)

#### 7.1 Adaptive Sampling
```cpp
class AdaptiveSampler {
    float variance_threshold = 0.01f;
    int min_samples = 4;
    int max_samples = 1024;
    
public:
    Vec3 sample_pixel(int x, int y, const Camera& camera, const Scene& scene) {
        std::vector<Vec3> samples;
        Vec3 mean(0, 0, 0);
        Vec3 variance(0, 0, 0);
        
        // Take minimum samples
        for (int i = 0; i < min_samples; ++i) {
            Vec3 sample = trace_pixel_sample(x, y, camera, scene);
            samples.push_back(sample);
            mean += sample;
        }
        mean /= min_samples;
        
        // Calculate initial variance
        for (const auto& sample : samples) {
            Vec3 diff = sample - mean;
            variance += diff * diff;
        }
        variance /= (min_samples - 1);
        
        // Add samples until variance is low enough or max samples reached
        int num_samples = min_samples;
        while (num_samples < max_samples) {
            float max_var = std::max({variance.x, variance.y, variance.z});
            if (max_var < variance_threshold) break;
            
            // Take additional samples
            int batch_size = std::min(num_samples, max_samples - num_samples);
            for (int i = 0; i < batch_size; ++i) {
                Vec3 sample = trace_pixel_sample(x, y, camera, scene);
                samples.push_back(sample);
                
                // Update running mean and variance
                update_statistics(mean, variance, sample, num_samples + i + 1);
            }
            num_samples += batch_size;
        }
        
        return mean;
    }
};
```

#### 7.2 Importance Sampling
```cpp
// Cosine-weighted hemisphere sampling for Lambertian materials
Vec3 sample_cosine_hemisphere(const Vec3& normal, float u1, float u2) {
    float cos_theta = sqrt(u1);
    float sin_theta = sqrt(1 - u1);
    float phi = 2 * M_PI * u2;
    
    Vec3 w = normal;
    Vec3 u = normalize(cross((abs(w.x) > 0.1 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)), w));
    Vec3 v = cross(w, u);
    
    return sin_theta * cos(phi) * u + sin_theta * sin(phi) * v + cos_theta * w;
}

// Multiple Importance Sampling for better convergence
Vec3 sample_direct_lighting_mis(const HitRecord& hit, const Scene& scene) {
    Vec3 result(0, 0, 0);
    
    for (const auto& light : scene.lights) {
        // Sample light source
        Vec3 light_dir;
        float light_pdf;
        Vec3 light_radiance = light.sample(hit.point, light_dir, light_pdf);
        
        if (light_pdf > 0) {
            Vec3 brdf = hit.material->evaluate_brdf(hit.wo, light_dir, hit.normal);
            float brdf_pdf = hit.material->pdf(hit.wo, light_dir, hit.normal);
            
            // Multiple Importance Sampling weight
            float weight = power_heuristic(light_pdf, brdf_pdf);
            
            result += weight * brdf * light_radiance * dot(hit.normal, light_dir) / light_pdf;
        }
        
        // Sample BRDF
        Vec3 brdf_dir;
        float brdf_pdf;
        Vec3 brdf = hit.material->sample(hit.wo, hit.normal, brdf_dir, brdf_pdf);
        
        if (brdf_pdf > 0) {
            Vec3 light_radiance = light.evaluate(hit.point, brdf_dir);
            float light_pdf = light.pdf(hit.point, brdf_dir);
            
            float weight = power_heuristic(brdf_pdf, light_pdf);
            
            result += weight * brdf * light_radiance * dot(hit.normal, brdf_dir) / brdf_pdf;
        }
    }
    
    return result;
}

float power_heuristic(float pdf1, float pdf2) {
    float f = pdf1 * pdf1;
    float g = pdf2 * pdf2;
    return f / (f + g);
}
```

## Performance Target Metrics

### Target Performance (1200×800, adaptive sampling)
- **Render Time**: 5-10 seconds (vs. current 3-5 minutes)
- **GPU Render Time**: 0.5-2 seconds
- **Memory Usage**: <200MB
- **CPU Utilization**: 90%+ on all cores
- **Quality**: Equivalent or better than current output

### Scalability Targets
- **4K Resolution (3840×2160)**: <60 seconds
- **Large Scenes (100K+ objects)**: Maintain interactive rates
- **Animation**: 1-10 seconds per frame for sequences

## Implementation Priority

### High Priority (Must Have)
1. ✅ **Multi-threading**: 4-8x speedup with minimal risk
2. ✅ **SIMD**: 2-4x speedup, cross-platform libraries available
3. ✅ **Memory optimization**: 2-3x speedup, improves all other optimizations

### Medium Priority (Should Have)
4. **Algorithm optimization**: 2-5x speedup, requires careful implementation
5. **BVH acceleration**: 10-100x speedup for complex scenes
6. **Adaptive sampling**: Variable speedup, improves quality/time trade-off

### Low Priority (Nice to Have)
7. **GPU acceleration**: 50-500x speedup but platform-dependent
8. **Advanced sampling**: Marginal improvement but better quality
9. **Denoising**: Post-processing quality improvement

## Risk Assessment

### Technical Risks
- **SIMD portability**: Different instruction sets across platforms
- **Thread safety**: Race conditions in shared data structures
- **GPU driver compatibility**: Platform-specific issues
- **Numerical precision**: Floating-point accuracy with optimizations

### Mitigation Strategies
- **Comprehensive testing**: Unit tests for all optimized components
- **Fallback implementations**: Scalar versions for unsupported platforms
- **Performance regression testing**: Automated benchmarks
- **Cross-platform CI**: Test on Windows, Linux, and macOS

## Success Metrics

### Performance Benchmarks
```cpp
struct BenchmarkSuite {
    void run_all_benchmarks() {
        benchmark_ray_sphere_intersection();
        benchmark_material_evaluation();
        benchmark_scene_traversal();
        benchmark_full_render();
    }
    
    void benchmark_full_render() {
        Scene test_scene = create_benchmark_scene();
        
        auto start = std::chrono::high_resolution_clock::now();
        Image result = render(test_scene);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Full render time: " << duration.count() << "ms\n";
        std::cout << "Rays per second: " << calculate_rays_per_second(test_scene, duration) << "\n";
    }
};
```

### Quality Metrics
- **Image comparison**: PSNR/SSIM against reference images
- **Noise levels**: Statistical analysis of sample variance
- **Convergence rate**: Time to reach acceptable quality threshold

This comprehensive performance plan targets a **20-100x overall speedup** through the combination of multi-threading, SIMD, memory optimization, and algorithmic improvements, with potential for 500x+ speedup when GPU acceleration is added.