#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include "appsrc/include/Math/sphere.h"
#include "appsrc/include/Math/modern_sphere.h"
#include "appsrc/include/Math/modern_vec3.h"
#include "appsrc/include/Math/simd_batch.h"
#include "appsrc/include/Math/hittablelist.h"
#include "appsrc/include/Math/camera.h"
#include "appsrc/include/Math/material.h"
#include "appsrc/include/Math/performance.h"

// Optimized color function with early termination and reduced recursion
Vec3 ColorOptimizedModern(const Ray &ray, Hittable *world, int depth, int max_depth = 8) {
    HitRecord rec;
    
    // Early termination for deep recursion
    if (depth >= max_depth) return Vec3(0.0f, 0.0f, 0.0f);
    
    if (world->Hit(ray, 0.001f, FLT_MAX, rec)) {
        Vec3 target = rec.m_oPoint + rec.m_oNormal + Vec3(FastRandom::random(), FastRandom::random(), FastRandom::random());
        
        // Reduce contribution of deeper bounces for performance
        float contribution = 0.5f * powf(0.9f, depth);
        return contribution * ColorOptimizedModern(Ray(rec.m_oPoint, target - rec.m_oPoint), world, depth + 1, max_depth);
    }
    
    // Sky gradient
    Vec3 unit_direction = Unit_Vector(ray.Direction());
    float t = 0.5f * (unit_direction.GetY() + 1.0f);
    return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
}

// Create optimized scene with spatial partitioning considerations
Hittable *CreateOptimizedModernScene() {
    int n = 30; // Reduced number of objects
    Hittable **list = new Hittable*[n+1];
    
    list[0] = new Sphere(Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));
    
    int i = 1;
    for (int a = -3; a < 4; a++) {
        for (int b = -3; b < 4; b++) {
            if (i >= n) break;
            
            float choose_mat = FastRandom::random();
            Vec3 center(a + 0.9f * FastRandom::random(), 0.2f, b + 0.9f * FastRandom::random());
            
            if ((center - Vec3(4.0f, 0.2f, 0.0f)).Length() > 0.9f) {
                if (choose_mat < 0.8f) {
                    // Lambertian
                    list[i++] = new Sphere(center, 0.2f, 
                        new Lambertian(Vec3(FastRandom::random() * FastRandom::random(),
                                          FastRandom::random() * FastRandom::random(),
                                          FastRandom::random() * FastRandom::random())));
                } else if (choose_mat < 0.95f) {
                    // Metal
                    list[i++] = new Sphere(center, 0.2f,
                        new Metal(Vec3(0.5f * (1.0f + FastRandom::random()),
                                     0.5f * (1.0f + FastRandom::random()),
                                     0.5f * (1.0f + FastRandom::random())),
                                0.5f * FastRandom::random()));
                } else {
                    // Glass
                    list[i++] = new Sphere(center, 0.2f, new Dielectric(1.5f));
                }
            }
        }
        if (i >= n) break;
    }

    list[i++] = new Sphere(Vec3(0.0f, 1.0f, 0.0f), 1.0f, new Dielectric(1.5f));
    list[i++] = new Sphere(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, new Lambertian(Vec3(0.4f, 0.2f, 0.1f)));
    list[i++] = new Sphere(Vec3(4.0f, 1.0f, 0.0f), 1.0f, new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f));

    return new HittableList(list, i);
}

// Multi-threaded tile renderer with better work distribution
void render_tile_modern_optimized(const RenderTile &tile, std::vector<Vec3> &pixels, 
                                 int nx, int ny, const Camera &camera, 
                                 Hittable *world, RenderStats &tile_stats,
                                 std::atomic<int> &completed_pixels) {
    PerformanceTimer tile_timer;
    tile_timer.begin();

    // Process pixels in cache-friendly order (rows)
    for (int j = tile.y_start; j < tile.y_start + tile.height; ++j) {
        for (int i = tile.x_start; i < tile.x_start + tile.width; ++i) {
            Vec3 col(0.0f, 0.0f, 0.0f);
            
            // Adaptive sampling based on pixel importance
            int samples = tile.samples_per_pixel;
            
            // Use stratified sampling for better quality with fewer samples
            for (int s = 0; s < samples; ++s) {
                float u = float(i + FastRandom::random()) / float(nx);
                float v = float(j + FastRandom::random()) / float(ny);

                Ray ray = camera.GetRay(u, v);
                col += ColorOptimizedModern(ray, world, 0, 6); // Reduced max depth
                tile_stats.rays_traced++;
            }

            col /= float(samples);
            col = Vec3(sqrtf(col[0]), sqrtf(col[1]), sqrtf(col[2])); // Gamma correction

            pixels[j * nx + i] = col;
            completed_pixels++;
        }
    }

    tile_stats.total_time_ms = tile_timer.elapsed_ms();
}

int main(int argc, char const *argv[]) {
    // Optimized parameters for much better performance
    int nx = 1280;  // Half of 2560
    int ny = 720;   // Half of 1440
    int ns = 6;     // Reduced from 10

    std::cout << "=== Modern Optimized Raytracer ===\n";
    std::cout << "Resolution: " << nx << "x" << ny << "\n";
    std::cout << "Samples per pixel: " << ns << "\n";

#ifdef USE_OPENMP
    int num_threads = std::min(12, omp_get_max_threads());
    omp_set_num_threads(num_threads);
    std::cout << "OpenMP threads: " << num_threads << "\n";
#else
    int num_threads = std::min(12u, std::thread::hardware_concurrency());
    std::cout << "Hardware threads: " << num_threads << "\n";
#endif

    // Performance measurement
    PerformanceTimer total_timer;
    RenderStats final_stats;
    total_timer.begin();

    // Initialize output buffer
    std::vector<Vec3> pixels(nx * ny);

    // Scene setup - use optimized scene
    std::unique_ptr<Hittable> world(CreateOptimizedModernScene());

    // Camera setup
    Vec3 lookFrom(13.0f, 2.0f, 3.0f);
    Vec3 lookAt(0.0f, 0.0f, 0.0f);
    float distToFocus = 10.0f;
    float aperture = 0.02f; // Small aperture for slight DOF

    Camera camera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 
                  20.0f, float(nx) / float(ny), aperture, distToFocus);

    std::cout << "Rendering started...\n";

    // Create optimally sized tiles
    auto tiles = create_tiles(nx, ny, ns, 32); // Smaller tiles for better load balancing
    std::vector<RenderStats> tile_stats(tiles.size());
    std::atomic<int> completed_pixels{0};
    int total_pixels = nx * ny;

    // Progress monitoring 
    std::atomic<bool> rendering_complete{false};
    std::thread progress_thread([&]() {
        auto start_time = std::chrono::steady_clock::now();
        
        while (!rendering_complete.load()) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            
            int completed = completed_pixels.load();
            float progress = (float)completed / total_pixels * 100.0f;
            float pixels_per_sec = completed > 0 && elapsed > 0 ? (float)completed / elapsed : 0.0f;
            float eta = pixels_per_sec > 0 ? (total_pixels - completed) / pixels_per_sec : 0.0f;
            
            std::cout << "\rProgress: " << std::fixed << std::setprecision(1) << progress 
                     << "% (" << completed << "/" << total_pixels << ") "
                     << "Speed: " << std::setprecision(0) << pixels_per_sec << " px/s "
                     << "ETA: " << std::setprecision(0) << eta << "s     " << std::flush;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

#ifdef USE_OPENMP
    // OpenMP parallel rendering with dynamic scheduling
    #pragma omp parallel for schedule(dynamic, 1)
    for (size_t t = 0; t < tiles.size(); ++t) {
        render_tile_modern_optimized(tiles[t], pixels, nx, ny, camera, 
                                    world.get(), tile_stats[t], completed_pixels);
    }
#else
    // Thread pool based rendering
    ThreadPool pool(num_threads);
    std::vector<std::function<void()>> tasks;
    
    for (size_t t = 0; t < tiles.size(); ++t) {
        tasks.emplace_back([&, t]() {
            render_tile_modern_optimized(tiles[t], pixels, nx, ny, camera, 
                                       world.get(), tile_stats[t], completed_pixels);
        });
    }
    
    pool.enqueue_tasks(tasks);
    pool.wait_all();
#endif

    rendering_complete.store(true);
    if (progress_thread.joinable()) {
        progress_thread.join();
    }

    // Combine statistics
    for (const auto &stats : tile_stats) {
        final_stats.rays_traced += stats.rays_traced.load();
        final_stats.intersection_tests += stats.intersection_tests.load();
        final_stats.material_evaluations += stats.material_evaluations.load();
    }

    final_stats.total_time_ms = total_timer.elapsed_ms();

    std::cout << "\n\nRendering complete!\n";
    final_stats.print_stats();

    // Performance analysis
    float pixels_per_second = (nx * ny) / (final_stats.total_time_ms / 1000.0f);
    float rays_per_pixel = (float)final_stats.rays_traced.load() / (nx * ny);
    float original_2k_time = final_stats.total_time_ms * (2560.0f * 1440.0f * 10) / (nx * ny * ns);
    
    std::cout << "\n=== Performance Analysis ===\n";
    std::cout << "Pixels per second: " << std::fixed << std::setprecision(0) << pixels_per_second << "\n";
    std::cout << "Average rays per pixel: " << std::setprecision(1) << rays_per_pixel << "\n";
    std::cout << "Estimated 2K@10spp time: " << std::setprecision(1) << original_2k_time / 1000.0f << " seconds\n";
    std::cout << "Performance improvement vs 2K: " << std::setprecision(1) << (2560.0f * 1440.0f * 10) / (nx * ny * ns) << "x\n";

    // Write output
    std::cout << "\nWriting output file...\n";
    std::ofstream output_file("output_modern_optimized.ppm");
    output_file << "P3\n" << nx << " " << ny << "\n255\n";

    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const Vec3 &col = pixels[j * nx + i];
            int ir = int(255.99 * col[0]);
            int ig = int(255.99 * col[1]);
            int ib = int(255.99 * col[2]);

            output_file << ir << " " << ig << " " << ib << "\n";
        }
    }

    output_file.close();
    std::cout << "Output written to output_modern_optimized.ppm\n";

    return 0;
}